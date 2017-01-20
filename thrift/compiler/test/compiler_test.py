from __future__ import print_function

import os
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
import traceback
import unittest

skip_py_generate = os.getenv('THRIFT_COMPILER_TEST_SKIP_PY_GENERATE')
thrift = os.getenv('THRIFT_COMPILER_BIN')
fixtures_root_dir = os.getenv('THRIFT_FIXTURES_DIR')
templates_dir = os.getenv('THRIFT_TEMPLATES_DIR')

def read_file(path):
    with open(path, 'r') as f:
        return f.read()

def read_lines(path):
    with open(path, 'r') as f:
        return f.readlines()

def read_directory_filenames(path):
    files = []
    for filename in os.listdir(path):
        files.append(filename)
    return files

def gen_find_recursive_files(path):
    for root, dirs, files in os.walk(path):
        for f in files:
            yield os.path.relpath(os.path.join(root, f), path)

def cp_dir(source_dir, dest_dir):
    if not os.path.isdir(dest_dir):
        os.makedirs(dest_dir, 0o700)
    for src in gen_find_recursive_files(source_dir):
        shutil.copy2(os.path.join(source_dir, src), dest_dir)

class CompilerTest(unittest.TestCase):

    MSG = " ".join([
        "One or more fixtures are out of sync with the thrift compiler.",
        "To sync them, build thrift and then run:",
        "`thrift/compiler/test/build_fixtures <build-dir>`, where",
        "<build-dir> is a path where the program `thrift/compiler/thrift`",
        "may be found.",
    ])

    def compare_code(self, path1, path2):
        gens = list(gen_find_recursive_files(path1))
        fixt = list(gen_find_recursive_files(path2))
        try:
            # Compare that the generated files are the same
            self.assertEqual(sorted(gens), sorted(fixt))
            for gen in gens:
                geng = read_file(os.path.join(path1, gen))
                genf = read_file(os.path.join(path2, gen))
                if geng != genf:
                    print(os.path.join(path1, gen), file=sys.stderr)
                # Compare that the file contents are equal
                self.assertMultiLineEqual(geng, genf)
        except Exception as e:
            print(self.MSG, file=sys.stderr)
            traceback.print_exc(file=sys.stderr)
            raise

    def setUp(self):
        tmp = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, tmp, True)
        self.tmp = tmp
        self.maxDiff = None

    def runTest(self, name):
        fixture_dir = os.path.join(fixtures_root_dir, name)
        # Copy source *.thrift files to temporary folder for relative code gen
        cp_dir(os.path.join(fixture_dir, 'src'), os.path.join(self.tmp, 'src'))

        languages = set()
        for cmd in read_lines(os.path.join(fixture_dir, 'cmd')):
            # Skip commented out commands
            if cmd[0] == '#':
                continue

            args = shlex.split(cmd.strip())
            # Get cmd language
            lang = args[0].rsplit(':', 1)[0] if ":" in args[0] else args[0]

            # Skip in cmake test. Python generator doesn't work
            if skip_py_generate == "True":
                if "cpp2" in lang or "schema" in lang:
                    continue

            # Add to list of generated languages
            languages.add(lang)

            # Fix cpp args
            if "cpp" in lang:
                path = os.path.join("thrift/compiler/test/fixtures", name)
                extra = "include_prefix=" + path
                join = "," if ":" in args[0] else ":"
                args[0] = args[0] + join + extra

            # Generate arguments to run binary
            args = [
                thrift, '-r',
                '--templates', templates_dir,
                '--gen', args[0],
                args[1]
            ]

            # Do not recurse in py generators due to a bug in the py generator
            # Remove once migration to mustache is done
            if ("cpp2" == lang) or ("schema" == lang):
                args.remove('-r')

            # Run thrift compiler and generate files
            subprocess.check_call(args, cwd=self.tmp, close_fds=True)

        # Compare generated code to fixture code
        for lang in languages:
            # Edit lang to find correct directory
            lang = lang.rsplit('_', 1)[0] if "android_lite" in lang else lang
            if "cpp2" not in lang:  # Remove 'mstch' if present expt in cpp2
                lang = lang.rsplit('_', 1)[1] if "mstch_" in lang else lang

            gen_code = os.path.join(self.tmp, 'gen-' + lang)
            fixture_code = os.path.join(fixture_dir, 'gen-' + lang)
            self.compare_code(gen_code, fixture_code)

        # Compare the generate code of cpp2 and mstch_cpp2 if both present
        if "cpp2" in languages and "mstch_cpp2" in languages:
            gen_code = os.path.join(self.tmp, 'gen-cpp2')
            fixture_code = os.path.join(fixture_dir, 'gen-mstch_cpp2')
            self.compare_code(gen_code, fixture_code)

def add_fixture(klazz, name):
    def test_method(self):
        self.runTest(name)
    test_method.__name__ = str('test_' + re.sub('[^0-9a-zA-Z]', '_', name))
    setattr(klazz, test_method.__name__, test_method)

fixtureNames = read_directory_filenames(fixtures_root_dir)
for name in fixtureNames:
    add_fixture(CompilerTest, name)

if __name__ == "__main__":
    unittest.main()
