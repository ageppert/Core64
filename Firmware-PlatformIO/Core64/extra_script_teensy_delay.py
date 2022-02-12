# Part of a workaround to delay opening the serial monitor: https://community.platformio.org/t/any-way-to-configure-timeout-for-upload-monitor/3812

Import("env")

def after_upload(source, target, env):
    print ("Delay while uploading...")
    import time
    time.sleep(2) # Number of seconds to wait before the serial monitor is going to open.
    print ("Done!")

env.AddPostAction("upload", after_upload)
