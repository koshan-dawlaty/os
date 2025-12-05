import gdb


# TODO: make this work for x86-64
class RegistersPrinter:

    def __init__(self, val):
        self.val = val
        if val.type.code == gdb.TYPE_CODE_PTR:
            self.val = val.dereference()
        else:
            self.val = val

    def to_string(self):
        return f"regs:(di:{self.val["edi"]} si:{self.val["esi"]} bp:{self.val["ebp"]} sp:{self.val["esp"]} bx:{self.val["ebx"]} dx:{self.val["edx"]} ecx:{self.val["ecx"]} ax:{self.val["eax"]})"


class TaskPrinter:

    def __init__(self, val):
        self.val = val
        if val.type.code == gdb.TYPE_CODE_PTR:
            self.val = val.dereference()
        else:
            self.val = val

    # The to_string method returns the value of the
    # si_signo attribute of the directory.

    def to_string(self):
        registers = self.val["regs"]
        return f"task: (stack:{self.val["stack"]} {self.val["regs"]})"


class ListPrinter:
    def __init__(self, val):
        self.val = val

        if val.type.code == gdb.TYPE_CODE_PTR:
            if val == 0:
                return None
            self.val = val.dereference()
        else:
            self.val = val

    def to_string(self):
        return f"{self.val}, {self.val["next"]}"


def my_pp_func(val):
    t = val.type.strip_typedefs()
    if t.code == gdb.TYPE_CODE_PTR:
        t = t.target()

    # if t.tag == "list":
    #     return ListPrinter(val)

    if t.code == gdb.TYPE_CODE_STRUCT and t.tag == "task":
        return TaskPrinter(val)

    if t.code == gdb.TYPE_CODE_STRUCT and t.tag == "registers":
        return RegistersPrinter(val)


gdb.pretty_printers.append(my_pp_func)
