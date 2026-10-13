
import json
from jsonschema import Draft202012Validator


def validate_json(json_file, schema_file):
    #load JSON
    try:
        with open(schema_file, "r", encoding="utf-8") as file:
            schema = json.load(file)
    except FileNotFoundError:
        print(f"Error not find: {schema_file}")
        return False
    except json.JSONDecodeError as error:
        print(f"Error invalid schema: {error}")
        return False

    #check valid schema
    try:
        Draft202012Validator.check_schema(schema)
    except Exception as error:
        print(f"Error invalid schema check : {error}")
        return False

    #load ESP JSON
    try:
        with open(json_file, "r", encoding="utf-8") as file:
            data = json.load(file)
    except FileNotFoundError:
        print(f"error not find : {json_file}")
        return False
    except json.JSONDecodeError as error:
        print(
            f"Error invalid json check"
            f"ligne {error.lineno}, colonne {error.colno} : {error.msg}"
        )
        return False

    # 4. check ESP JSON
    validator = Draft202012Validator(schema)
    errors = sorted(
        validator.iter_errors(data),
        key=lambda error: list(map(str, error.absolute_path))
    )

    if errors:
        print("Schema check : ESP JSON invalid:")
        for error in errors:
            path = "$"
            for part in error.absolute_path:
                if isinstance(part, int):
                    path += f"[{part}]"
                else:
                    path += f".{part}"

            print(f"- {path} : {error.message}")

        return False

    print("Schema check : ESP JSON valid")
    return True


if __name__ == "__main__":
    validate_json("esp.json", "schema.json")
