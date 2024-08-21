Import("env")
import os
from platformio.builder.tools.pioupload import AutodetectUploadPort

platform = env.PioPlatform()

AutodetectUploadPort(env)
upload_port = env.subst('$UPLOAD_PORT')

def get_esptoolpy_reset_flags(resetmethod):
    # no dtr, no_sync
    resets = ("default_reset", "hard_reset")
    if resetmethod == "nodemcu":
        # dtr
        resets = ("default_reset", "hard_reset")
    elif resetmethod == "ck":
        # no dtr
        resets = ("CustomReset", "hard_reset")

    return ["--before", resets[0], "--after", resets[1]]

reset_flags = ' '.join(get_esptoolpy_reset_flags(env.subst("$UPLOAD_RESETMETHOD")))

esptool = os.path.join(platform.get_package_dir("tool-esptoolpy"), "esptool.py")

esptool_cmd = f'$PYTHONEXE "{esptool}" --port {upload_port} {reset_flags} --no-stub run' 
print(esptool_cmd); 
# Multiple actions
env.AddCustomTarget(
    name="reset2",
    dependencies=None,
    actions=[
        esptool_cmd
    ],
    title="Reset ESP8266",
    description="Resets the ESP8266 board"
)