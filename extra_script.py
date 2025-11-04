import os
from SCons.Script import DefaultEnvironment

env = DefaultEnvironment()
build_dir = env.subst("$BUILD_DIR")
project_dir = env.subst("$PROJECT_DIR")

# Normal build output file
firmware_bin = os.path.join(build_dir, "firmware.bin")

# This will be your OTA version (without bootloader/partitions)
ota_output = os.path.join(project_dir, "firmware_ota.bin")

def copy_ota_file(source, target, env):
    print("🔧 Creating OTA firmware image:", ota_output)
    os.system(f"cp {firmware_bin} {ota_output}")

env.AddPostAction(firmware_bin, copy_ota_file)