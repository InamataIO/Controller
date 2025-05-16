import subprocess

Import("env")

RUN_PARAMS = {"stdout": subprocess.PIPE, "stderr": subprocess.PIPE, "text": True}


def is_main_branch():
    main_result = subprocess.run(["git", "rev-parse", "origin/main"], **RUN_PARAMS)
    if main_result.returncode != 0:
        raise Exception(f"Error: {main_result.stderr}")
    head_result = subprocess.run(["git", "rev-parse", "HEAD"], **RUN_PARAMS)
    if head_result.returncode != 0:
        raise Exception(f"Error: {head_result.stderr}")
    return main_result.stdout == head_result.stdout


def get_git_commit_hash():
    result = subprocess.run(["git", "rev-parse", "HEAD"], **RUN_PARAMS)
    if result.returncode != 0:
        raise Exception(f"Error: {result.stderr}")
    commit_hash = result.stdout.strip()
    return commit_hash[:7]


name = env.GetProjectOption("custom_firmware_name")
version = env.GetProjectOption("custom_firmware_version")
if "@" in name or "@" in version:
    raise ValueError(
        "'custom_firmware_name' and 'custom_firmware_version' may not contain '@' characters"
    )
name_version = f"{name}@{version}"

# If not on main branch add commit hash
if not is_main_branch():
    name_version = f"{name_version}-{get_git_commit_hash()}"

print(f"Building version: {name_version}")
env.Replace(PROGNAME=name_version)
env.Append(CPPDEFINES={"FIRMWARE_VERSION": f'\\"{name_version}\\"'})
