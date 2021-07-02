
import setuptools

MODULES = ["lzss", "gx2", "endian", "yaz0", "audio"]

description = \
	"C++ extension with functions for which python is too slow."

long_description = description

extensions = []
for module in MODULES:
	extension = setuptools.Extension(
		name = "ninty.%s" %module,
		sources = ["src/module_%s.cpp" %module]
	)
	extensions.append(extension)

setuptools.setup(
	name = "ninty",
	version = "0.0.4",
	description = description,
	long_description = long_description,
	author = "Yannik Marchand",
	author_email = "ymarchand@me.com",
	url = "https://github.com/kinnay/ninty",
	license = "GPLv3",
	ext_modules = extensions
)
