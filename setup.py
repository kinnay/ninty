
from distutils.core import Extension, setup

MODULES = ["lzss", "gx2", "endian"]

extensions = []
for module in MODULES:
	extension = Extension(
		name = "wiiulib.%s" %module,
		sources = ["src/module_%s.cpp" %module],
	)
	extensions.append(extension)

setup(
	name = "wiiulib",
	ext_modules = extensions
)
