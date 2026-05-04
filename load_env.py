Import("env")
import os

env_path = os.path.join(env["PROJECT_DIR"], ".env")
if not os.path.exists(env_path):
    print("WARNING: .env not found — copy .env.example to .env and fill in WiFi creds")
else:
    with open(env_path) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            key, val = line.split("=", 1)
            key, val = key.strip(), val.strip().strip('"').strip("'")
            escaped = val.replace("\\", "\\\\").replace('"', '\\"')
            env.Append(BUILD_FLAGS=[f'-D{key}="\\"{escaped}\\""'])
