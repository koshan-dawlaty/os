import re
import time
import argparse
import sys
import typing


parser = argparse.ArgumentParser(prog="tap.py", description="", epilog="")
parser.add_argument("-v", "--verbose", action="store_true")
args = parser.parse_args()


class TestResult:

    def __init__(self, failed: bool, message: str) -> None:
        self.failed = failed
        self.message = message


FAIL = "not ok"
NOT_FAIL = "ok"


def parse_result(s: str) -> TestResult:

    failed = None

    if s[: len(NOT_FAIL)] == NOT_FAIL:
        failed = False
    elif s[: len(FAIL)] == FAIL:
        failed = True
    else:
        print(s)
        exit(1)

    message = s[s.index("-") + 1 : -1]
    if message[0] == " ":
        message = message[1:]

    return TestResult(failed, message)


def parse_plan(s: str) -> typing.Tuple[int, int]:

    plan = s
    match1 = re.search(r"\d+", plan)

    if match1 == None:
        print(f"first value in plan: {match1} was invalid")
        exit(1)
    min_str = match1.group()

    min = int(min_str)

    match2 = re.search(r"\d+", plan[(len(min_str) + 2) :])
    if match2 == None:
        print("second value in plan was invalid")
        exit(1)

    max = int(match2.group())

    return (min, max)


def format_time(time: float) -> str:
    hours = int(time // 3600)
    minutes = int((time % 3600) // 60)
    seconds = int(time % 60)
    miliseconds = int((time % 1) * 1000)

    output = ""

    if hours > 0:
        output += str(hours) + " h "

    if minutes > 0:
        output += str(minutes) + " m "

    if seconds > 0:
        output += str(seconds) + " s "

    if miliseconds > 0:
        output += str(miliseconds) + " ms"
    elif output == "":
        return "0 ms"

    return output


def read_stdin():

    i = 0

    plan = None
    failed = []

    start = time.time()
    total_start = time.time()

    for line in sys.stdin:

        # line = line_bytes.decode()

        if i == 0:
            plan = parse_plan(line)
            start = time.time()
            if plan[0] != 1:
                print("the first value in the plan was not 1")
                exit(1)

        else:

            result = parse_result(line)

            elapsed = format_time(time.time() - start)

            display_result(result, elapsed)

            if result.failed:
                failed.append(result)

            start = time.time()

        i += 1

        if i == plan[1] + 1:
            display_final(failed, format_time(time.time() - total_start))
            exit(0)


def display_result(r: TestResult, time: str):

    if not r.failed and not args.verbose:
        return

    message = "test " + r.message + " "

    if r.failed:
        message += "failed"
    elif args.verbose:
        message += "did not fail"

    message += ", ran in " + time
    print(message)


def display_final(failed: typing.List[TestResult], time: str):

    if len(failed) == 0:
        print("all tests passed")
    else:
        print(str(len(failed)) + " tests failed")

    print("tests ran in " + time)


read_stdin()
