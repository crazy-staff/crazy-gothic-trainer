# replace the text between two markers (start inclusive, end exclusive) in a file
import sys
def repl(path, start, end, new):
    s = open(path).read()
    a = s.index(start)
    b = s.index(end, a + len(start)) if end else len(s)
    s = s[:a] + new + s[b:]
    open(path, 'w').write(s)
