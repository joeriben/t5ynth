"""Simple and tiny yield-based trampoline implementation.
"""


class TailCall(BaseException):
    "Raised to signal a tail call to trampoline()."

    def __init__(self, call):
        self.call = call


def trampoline(call):
    "Invoke a generator for a trampolined function call."

    stack = [call]
    retval = None
    exception = None
    while stack:
        try:
            if exception is not None:
                ex, exception = exception, None
                res = stack[-1].throw(ex)
            elif retval is None:
                # We use next() here for nicer exceptions if an invalid value
                # was yielded.
                res = next(stack[-1])
            else:
                value, retval = retval, None
                res = stack[-1].send(value)
            stack.append(res)
        except StopIteration as e:
            stack.pop()
            retval = e.value
        except TailCall as e:
            stack[-1] = e.call
        except BaseException as e:
            stack.pop()
            exception = e
    if exception is not None:
        raise exception
    return retval


# vim:set sw=4 et:
