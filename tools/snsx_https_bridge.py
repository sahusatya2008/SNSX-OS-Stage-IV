#!/usr/bin/env python3
import argparse
import errno
import ssl
import urllib.parse
import urllib.request
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer


DEFAULT_HOST = "0.0.0.0"
DEFAULT_PORT = 8787
MAX_BYTES = 262144


def bridge_healthcheck(host: str, port: int) -> bool:
    probe_host = "127.0.0.1" if host in ("0.0.0.0", "::") else host
    try:
        with urllib.request.urlopen(f"http://{probe_host}:{port}/health", timeout=2) as response:
            body = response.read(128).decode("utf-8", errors="replace")
            return response.status == 200 and "SNSX HTTPS bridge is ready." in body
    except Exception:  # noqa: BLE001
        return False


class SnsxHttpsBridge(BaseHTTPRequestHandler):
    server_version = "SNSXHTTPSBridge/0.1"

    def do_GET(self):
        parsed = urllib.parse.urlparse(self.path)
        if parsed.path == "/health":
            self._write_text(200, "SNSX HTTPS bridge is ready.\n")
            return

        if parsed.path != "/fetch":
            self._write_text(404, "Unknown SNSX bridge route.\n")
            return

        params = urllib.parse.parse_qs(parsed.query, keep_blank_values=False)
        target = params.get("url", [""])[0].strip()
        if not target:
            self._write_text(400, "Missing url query parameter.\n")
            return
        if not target.startswith(("https://", "http://")):
            self._write_text(400, "SNSX bridge only accepts http:// or https:// targets.\n")
            return

        request = urllib.request.Request(
            target,
            headers={
                "User-Agent": "SNSX-HTTPS-Bridge/0.1",
                "Accept": "text/html, text/plain;q=0.9, */*;q=0.1",
                "Accept-Encoding": "identity",
            },
        )
        context = ssl.create_default_context()

        try:
            with urllib.request.urlopen(request, timeout=20, context=context) as response:
                content_type = response.headers.get_content_type() or "text/plain"
                charset = response.headers.get_content_charset() or "utf-8"

                if not (content_type.startswith("text/") or content_type in ("application/json", "application/xml")):
                    self._write_text(
                        415,
                        f"Unsupported content type for SNSX browser bridge: {content_type}\n",
                    )
                    return

                raw = response.read(MAX_BYTES)
                text = raw.decode(charset, errors="replace")
                self._write_text(200, text, content_type=f"{content_type}; charset=utf-8")
        except Exception as exc:  # noqa: BLE001
            self._write_text(502, f"SNSX bridge fetch failed.\nTarget: {target}\nError: {exc}\n")

    def log_message(self, fmt, *args):
        print(f"[SNSX-HTTPS-BRIDGE] {self.address_string()} - {fmt % args}")

    def _write_text(self, status, text, content_type="text/plain; charset=utf-8"):
        payload = text.encode("utf-8", errors="replace")
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(payload)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(payload)


def main():
    parser = argparse.ArgumentParser(description="SNSX HTTPS bridge for VirtualBox NAT guests")
    parser.add_argument("--host", default=DEFAULT_HOST)
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    args = parser.parse_args()

    try:
        server = ThreadingHTTPServer((args.host, args.port), SnsxHttpsBridge)
    except OSError as exc:
        if exc.errno == errno.EADDRINUSE and bridge_healthcheck(args.host, args.port):
            print(f"SNSX HTTPS bridge is already running on http://{args.host}:{args.port}")
            return
        if exc.errno == errno.EADDRINUSE:
            raise SystemExit(
                f"Port {args.port} is already in use by another process. "
                "Stop it first or free the port before starting the SNSX HTTPS bridge."
            ) from exc
        raise

    print(f"SNSX HTTPS bridge listening on http://{args.host}:{args.port}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
