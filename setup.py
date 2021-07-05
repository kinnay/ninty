
import setuptools

MODULES = {
	"lzss": ["src/module_lzss.cpp"],
	"gx2": ["src/module_gx2.cpp"],
	"endian": ["src/module_endian.cpp"],
	"yaz0": ["src/module_yaz0.cpp"],
	"audio": [
		"src/module_audio.cpp",
		"src/dsptool/encoder.cpp"
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
		include_dirs = ["src"]
	)
	extensions.append(extension)

setuptools.setup(
	name = "ninty",
	version = "0.0.7",
	description = description,
	long_description = long_description,
	author = "Yannik Marchand",
	author_email = "ymarchand@me.com",
	url = "https://github.com/kinnay/ninty",
	license = "GPLv3",
	ext_modules = extensions
)
