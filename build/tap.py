import re
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

    message = s[: s.index("-")]

    return TestResult(failed, message)


def parse_plan(s: str) -> typing.Tuple[int, int]:

    plan = s
    match1 = re.search(r"\d+", plan)

    if match1 == None:
        print("first value in plan was invalid")
        exit(1)
    min_str = match1.group()

    min = int(min_str)

    match2 = re.search(r"\d+", plan[(len(min_str) + 2) :])
    if match2 == None:
        print("second value in plan was invalid")
        exit(1)

    max = int(match2.group())

    return (min, max)


def read_stdin():

    i = 0

    plan = None
    failed = []

    for line in sys.stdin:

        # line = line_bytes.decode()

        if i == 0:
            plan = parse_plan(line)
            if plan[0] != 1:
                print("the first value in the plan was not 1")
                exit(1)

        else:

            result = parse_result(line)

            display_result(result)

            if result.failed:
                failed.append(result)

        i += 1

        if i == plan[1] + 1:
            display_final(failed)
            exit(0)


def display_result(r: TestResult):

    if r.failed:
        print("test " + r.message + " failed")
    elif args.verbose:
        print("test " + r.message + " did not fail")


def display_final(failed: typing.List[TestResult]):

    if len(failed) == 0:
        print("all tests passed")
        return
    else:
        print("all tests failed")

    # for test in failed:
    #     display_result(test)


read_stdin()
