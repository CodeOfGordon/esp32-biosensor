import csv
import time

from fastapi import FastAPI
from fastapi.responses import FileResponse
from pydantic import BaseModel, Field

app = FastAPI()
history = []


class Reading(BaseModel):
    bpm: int = Field(ge=30, le=220)
    prediction: float = 0


@app.post("/bpm")
def post_bpm(r: Reading):
    row = {"t": round(time.time(), 1), "bpm": r.bpm, "prediction": r.prediction}
    history.append(row)
    with open("server/bpm_log.csv", "a", newline="") as f:
        csv.writer(f).writerow([row["t"], row["bpm"]])
    return {"ok": True}


@app.get("/bpm")
def get_bpm():
    return history[-120:]


@app.get("/")
def dashboard():
    return FileResponse("server/index.html")
