
import setuptools
import os

def walk(path):
	files = []
	for dirpath, dirnames, filenames in os.walk(path):
		for filename in filenames:
			if filename.endswith(".cpp"):
				files.append(os.path.join(dirpath, filename))
	return files

MODULES = {
	"lzss": ["src/module_lzss.cpp"],
	"gx2": [
		"src/module_gx2.cpp",
		*walk("src/addrlib"),
		*walk("src/gx2")
	],
	"endian": ["src/module_endian.cpp"],
	"yaz0": ["src/module_yaz0.cpp"],
	"audio": [
		"src/module_audio.cpp",
		*walk("src/dsptool")
	]
}

description = \
	"C++ extension with functions for which python is too slow."

long_description = description

extensions = []
for name, files in MODULES.items():
	extension = setuptools.Extension(
		name = "ninty.%s" %name,
		sources = files,
		include_dirs = ["src", "src/addrlib"]
	)
	extensions.append(extension)

setuptools.setup(
	name = "ninty",
	version = "0.0.9",
	description = description,
	long_description = long_description,
	author = "Yannik Marchand",
	author_email = "ymarchand@me.com",
	url = "https://github.com/kinnay/ninty",
	license = "GPLv3",
	ext_modules = extensions
)
