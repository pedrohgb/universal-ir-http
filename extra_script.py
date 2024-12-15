# Código obtido em PlatformIO Community, autoria do usuário greyivy (2020)
# Fonte: https://community.platformio.org/t/project-fails-to-upload-to-esp32-on-platformio-works-on-arduino-ide/9813

Import("env")

import subprocess
from os.path import expanduser, join

home = expanduser("~")
esptool = join(home, ".platformio", "packages", "tool-esptoolpy", "esptool.py")

def on_upload(source, target, env):
    file = str(source[0])
    subprocess.run(["python", esptool, "write_flash", "-z", "0x10000", file])

env.Replace(UPLOADCMD=on_upload)