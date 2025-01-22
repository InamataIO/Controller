import subprocess

Import("env")

RUN_PARAMS = {"stdout": subprocess.PIPE, "stderr": subprocess.PIPE, "text": True}


def get_git_branch():
    try:
        result = subprocess.run(
            ["git", "rev-parse", "--abbrev-ref", "HEAD"], **RUN_PARAMS
        )
        if result.returncode != 0:
            raise Exception(f"Error: {result.stderr}")
        return result.stdout.strip()
    except Exception as e:
        return str(e)


def get_git_commit_hash():
    try:
        result = subprocess.run(["git", "rev-parse", "HEAD"], **RUN_PARAMS)
        if result.returncode != 0:
            raise Exception(f"Error: {result.stderr}")
        commit_hash = result.stdout.strip()
        return commit_hash[:7]
    except Exception as e:
        return str(e)


name = env.GetProjectOption("custom_firmware_name")
version = env.GetProjectOption("custom_firmware_version")
if "@" in name or "@" in version:
    raise ValueError(
        "'custom_firmware_name' and 'custom_firmware_version' may not contain '@' characters"
    )
name_version = f"{name}@{version}"

# If not on main branch add commit hash
if get_git_branch() != "main":
    name_version = f"{name_version}-{get_git_commit_hash()}"

env.Replace(PROGNAME=name_version)
env.Append(CPPDEFINES={"FIRMWARE_VERSION": f'\\"{name_version}\\"'})
