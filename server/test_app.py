import os
import tempfile

from fastapi.testclient import TestClient

import app

os.chdir(tempfile.mkdtemp())
os.mkdir("server")
client = TestClient(app.app)

assert client.post("/bpm", json={"bpm": 72, "prediction": 75.5}).status_code == 200
assert client.post("/bpm", json={"bpm": 5}).status_code == 422
last = client.get("/bpm").json()[-1]
assert last["bpm"] == 72 and last["prediction"] == 75.5
assert open("server/bpm_log.csv").read().strip().endswith(",72")
print("ok")
