
import sys
import json
from pathlib import Path
from jsonschema import Draft202012Validator

SCHEMA_FILE = Path(__file__).resolve().with_name("schema.json")

try:
    with SCHEMA_FILE.open("r", encoding="utf-8") as file:
        schema = json.load(file)

    Draft202012Validator.check_schema(schema)
    validator = Draft202012Validator(schema)

except Exception as error:
    print(f"Erreur de chargement du schéma : {error}", file=sys.stderr)
    sys.exit(1)

for raw_json in sys.stdin:
    if not raw_json.strip():
        continue

    try:
        data = json.loads(raw_json)
        errors = sorted(
            validator.iter_errors(data),
            key=lambda error: list(map(str, error.absolute_path))
        )

        if errors:
            for error in errors:
                path = "$"
                for part in error.absolute_path:
                    if isinstance(part, int):
                        path += f"[{part}]"
                    else:
                        path += f".{part}"

                print(f"{path} : {error.message}", file=sys.stderr, flush=True)

            continue

        print(json.dumps(data, ensure_ascii=False), flush=True)

    except Exception as error:
        print(f"JSON invalide : {error}", file=sys.stderr, flush=True)
