#!/usr/bin/env python3

import subprocess
import os
import json
from datetime import datetime

# Set script path
script_path = "test.py"  # change if named differently

# Define logging directory
log_dir = "test_logs"
os.makedirs(log_dir, exist_ok=True)

# Create a timestamped subdirectory
timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
run_dir = os.path.join(log_dir, timestamp)
os.makedirs(run_dir, exist_ok=True)

# Output files
stdout_path = os.path.join(run_dir, "stdout.txt")
stderr_path = os.path.join(run_dir, "stderr.txt")
returncode_path = os.path.join(run_dir, "return_code.json")

# Run the test script
with open(stdout_path, "w") as out, open(stderr_path, "w") as err:
    process = subprocess.run(["python3", script_path], stdout=out, stderr=err)

# Save return code
with open(returncode_path, "w") as f:
    json.dump({"return_code": process.returncode}, f, indent=2)

print(f"Test run saved in: {run_dir}")
