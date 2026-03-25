"""T5ynth Backend -- Stable Audio T5 Embedding Space Navigation

Standalone server extracted from AI4ArtsEd GPU Service.
Binds ONLY to localhost (127.0.0.1) -- no external access.
"""
import logging
import sys
from pathlib import Path

# Ensure backend/ is on sys.path so 'from config import ...' works
_backend_dir = str(Path(__file__).resolve().parent)
if _backend_dir not in sys.path:
    sys.path.insert(0, _backend_dir)

from flask import Flask, jsonify
from routes.cross_aesthetic_routes import cross_aesthetic_bp

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s [%(name)s] %(levelname)s: %(message)s',
)

app = Flask(__name__)
app.register_blueprint(cross_aesthetic_bp)


@app.route('/health')
def health():
    return jsonify({"status": "ok"})


if __name__ == '__main__':
    from config import HOST, PORT
    print(f"T5ynth backend starting on {HOST}:{PORT}")
    app.run(host=HOST, port=PORT, debug=False)
