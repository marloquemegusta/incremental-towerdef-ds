#!/usr/bin/env python3
"""Serve the compiled balance file to a local development client."""
from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
from pathlib import Path
import argparse

class Handler(SimpleHTTPRequestHandler):
    def log_message(self, fmt, *args):
        print(fmt % args)

def main():
    p = argparse.ArgumentParser()
    p.add_argument('--directory', type=Path, default=Path('artifacts'))
    p.add_argument('--host', default='0.0.0.0')
    p.add_argument('--port', type=int, default=5000)
    a = p.parse_args()
    directory = a.directory.resolve()
    if not (directory / 'towerds_balance.bin').is_file():
        raise SystemExit('Falta towerds_balance.bin; ejecuta compile_balance.py primero')
    class RootedHandler(Handler):
        def __init__(self, *args, **kwargs):
            super().__init__(*args, directory=str(directory), **kwargs)
    print(f'Serving {directory / "towerds_balance.bin"} on {a.host}:{a.port}')
    ThreadingHTTPServer((a.host, a.port), RootedHandler).serve_forever()

if __name__ == '__main__':
    main()
