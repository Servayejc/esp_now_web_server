import env

def add_prefix(source,taget,env):
    return "[PROJECT_A]" + source
env.AddPostAction("monitor",add_prefix)
