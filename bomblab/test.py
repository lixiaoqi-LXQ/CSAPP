import subprocess

SUM = 6

EXEC = "./bomb"
ARGS = "passwd"

CMD = [EXEC, ARGS]

def check(lis: list):
    lis = [str(k) for k in lis]
    _in = " ".join(lis) + "\n"

    res = subprocess.run(CMD, input=_in.encode("utf8"), capture_output=True)
    if res.returncode == 0:
        print(_in)


def check_helper(lis):
    if len(lis) == SUM:
        #  print(lis)
        check(lis);
        return
    for i in range(SUM + 1):
        if i not in lis:
            lis.append(i)
            check_helper(lis)
            lis.pop()

if __name__ == '__main__':
    check_helper([])
