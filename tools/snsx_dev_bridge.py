#!/usr/bin/env python3
import argparse
import errno
import pathlib
import subprocess
import tempfile
import urllib.parse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer


DEFAULT_HOST = "0.0.0.0"
DEFAULT_PORT = 8790
MAX_BYTES = 65536
ROOT = pathlib.Path(__file__).resolve().parent.parent
SNSX_BIN = ROOT / "Programming Lang" / "target" / "release" / "snsx"


def bridge_healthcheck(host: str, port: int) -> bool:
    import urllib.request

    probe_host = "127.0.0.1" if host in ("0.0.0.0", "::") else host
    try:
        with urllib.request.urlopen(f"http://{probe_host}:{port}/health", timeout=2) as response:
            body = response.read(128).decode("utf-8", errors="replace")
            return response.status == 200 and "SNSX Dev bridge is ready." in body
    except Exception:  # noqa: BLE001
        return False


def detect_language(language: str, path_hint: str) -> str:
    language = (language or "").strip().lower()
    if language:
        return language

    suffix = pathlib.Path(path_hint or "").suffix.lower()
    return {
        ".snsx": "snsx",
        ".py": "python",
        ".js": "javascript",
        ".c": "c",
        ".cpp": "cpp",
        ".cc": "cpp",
        ".cxx": "cpp",
        ".html": "html",
        ".htm": "html",
        ".css": "css",
    }.get(suffix, "text")


def extension_for_language(language: str) -> str:
    return {
        "snsx": ".snsx",
        "python": ".py",
        "javascript": ".js",
        "c": ".c",
        "cpp": ".cpp",
        "html": ".html",
        "css": ".css",
    }.get(language, ".txt")


def format_report(action: str, language: str, path_hint: str, command: list[str], result: subprocess.CompletedProcess | None,
                  note: str = "") -> str:
    lines = [
        "SNSX Dev Bridge",
        "===============",
        f"Action: {action}",
        f"Language: {language}",
        f"Path: {path_hint or '(unspecified)'}",
    ]
    if command:
        lines.append(f"Command: {' '.join(command)}")
    if result is not None:
        lines.append(f"Exit: {result.returncode}")
    if note:
        lines.append(f"Note: {note}")
    lines.append("")
    if result is not None and result.stdout:
        lines.append(result.stdout)
    if result is not None and result.stderr:
        if result.stdout:
            lines.append("")
        lines.append(result.stderr)
    return "\n".join(lines).rstrip() + "\n"


def run_command(command: list[str], cwd: pathlib.Path, timeout: int = 30) -> subprocess.CompletedProcess:
    return subprocess.run(  # noqa: PLW1510
        command,
        cwd=str(cwd),
        capture_output=True,
        text=True,
        timeout=timeout,
        check=False,
    )


def execute_source(action: str, language: str, path_hint: str, source: str) -> tuple[int, str]:
    language = detect_language(language, path_hint)
    suffix = extension_for_language(language)
    base_name = pathlib.Path(path_hint or f"main{suffix}").name or f"main{suffix}"

    with tempfile.TemporaryDirectory(prefix="snsx-dev-") as temp_dir_name:
        temp_dir = pathlib.Path(temp_dir_name)
        source_path = temp_dir / base_name
        source_path.write_text(source, encoding="utf-8")

        if language == "snsx":
            if not SNSX_BIN.exists():
                return 500, "SNSX Dev Bridge\n===============\nSNSX CLI binary is missing.\n"
            if action == "run":
                command = [str(SNSX_BIN), "run", "--file", str(source_path)]
                result = run_command(command, temp_dir, timeout=40)
                return (200 if result.returncode == 0 else 500), format_report(action, language, path_hint, command, result)
            out_path = temp_dir / "build.snsxbc"
            command = [str(SNSX_BIN), "build", "--file", str(source_path), "--target", "vm", "--out", str(out_path)]
            result = run_command(command, temp_dir, timeout=40)
            note = f"Artifact: {out_path.name}" if out_path.exists() else ""
            return (200 if result.returncode == 0 else 500), format_report(action, language, path_hint, command, result, note)

        if language == "python":
            command = ["python3", str(source_path)] if action == "run" else ["python3", "-m", "py_compile", str(source_path)]
            result = run_command(command, temp_dir)
            return (200 if result.returncode == 0 else 500), format_report(action, language, path_hint, command, result)

        if language == "javascript":
            command = ["node", str(source_path)] if action == "run" else ["node", "--check", str(source_path)]
            result = run_command(command, temp_dir)
            return (200 if result.returncode == 0 else 500), format_report(action, language, path_hint, command, result)

        if language in {"c", "cpp"}:
            compiler = "clang" if language == "c" else "clang++"
            binary_path = temp_dir / "program"
            compile_command = [compiler, str(source_path), "-o", str(binary_path)]
            compile_result = run_command(compile_command, temp_dir)
            if compile_result.returncode != 0 or action == "build":
                note = f"Artifact: {binary_path.name}" if compile_result.returncode == 0 else ""
                return (200 if compile_result.returncode == 0 else 500), format_report(
                    action, language, path_hint, compile_command, compile_result, note
                )
            run_command_line = [str(binary_path)]
            run_result = run_command(run_command_line, temp_dir)
            return (200 if run_result.returncode == 0 else 500), format_report(
                action, language, path_hint, compile_command + ["&&", str(binary_path)], run_result
            )

        if language in {"html", "css"}:
            note = "HTML/CSS are preview workspaces. Open snsx://preview in SNSX Browser after editing."
            return 200, format_report(action, language, path_hint, [], None, note) + "\n" + source[:MAX_BYTES] + "\n"

        return 400, format_report(action, language, path_hint, [], None, "Unsupported language for this bridge.")


class SnsxDevBridge(BaseHTTPRequestHandler):
    server_version = "SNSXDevBridge/0.1"

    def do_GET(self):
        parsed = urllib.parse.urlparse(self.path)
        if parsed.path == "/health":
            self._write_text(200, "SNSX Dev bridge is ready.\n")
            return
        self._write_text(404, "Unknown SNSX dev bridge route.\n")

    def do_POST(self):
        parsed = urllib.parse.urlparse(self.path)
        if parsed.path not in {"/run", "/build"}:
            self._write_text(404, "Unknown SNSX dev bridge route.\n")
            return

        params = urllib.parse.parse_qs(parsed.query, keep_blank_values=False)
        language = params.get("lang", [""])[0]
        path_hint = params.get("path", [""])[0]
        content_length = int(self.headers.get("Content-Length", "0") or "0")
        if content_length <= 0 or content_length > MAX_BYTES:
            self._write_text(400, "Missing or oversized request body.\n")
            return

        raw = self.rfile.read(content_length)
        source = raw.decode("utf-8", errors="replace")
        status, report = execute_source(parsed.path.lstrip("/"), language, path_hint, source)
        self._write_text(status, report)

    def log_message(self, fmt, *args):
        print(f"[SNSX-DEV-BRIDGE] {self.address_string()} - {fmt % args}")

    def _write_text(self, status: int, text: str, content_type: str = "text/plain; charset=utf-8"):
        payload = text.encode("utf-8", errors="replace")
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(payload)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(payload)


def main():
    parser = argparse.ArgumentParser(description="SNSX host-backed development bridge")
    parser.add_argument("--host", default=DEFAULT_HOST)
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    args = parser.parse_args()

    try:
        server = ThreadingHTTPServer((args.host, args.port), SnsxDevBridge)
    except OSError as exc:
        if exc.errno == errno.EADDRINUSE and bridge_healthcheck(args.host, args.port):
            print(f"SNSX Dev bridge is already running on http://{args.host}:{args.port}")
            return
        if exc.errno == errno.EADDRINUSE:
            raise SystemExit(
                f"Port {args.port} is already in use by another process. "
                "Stop it first or free the port before starting the SNSX Dev bridge."
            ) from exc
        raise

    print(f"SNSX Dev bridge listening on http://{args.host}:{args.port}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
