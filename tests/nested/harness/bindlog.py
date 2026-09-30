import json

from kwinsession import konveyor, wait_for


def last_bind():
    return json.loads(konveyor("LastBind"))


def fired(press, timeout=5):
    before = last_bind()["count"]
    press()
    after = wait_for(lambda: last_bind() if last_bind()["count"] > before else None, timeout)
    return after["key"] if after else None


def missed(press, sentinel, sentinel_key):
    before = last_bind()["count"]
    press()
    sentinel()
    after = wait_for(lambda: last_bind() if last_bind()["count"] > before and last_bind()["key"] == sentinel_key else None, 10)
    return after is not None and after["count"] == before + 1
