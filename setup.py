from setuptools import setup, Extension

with open("README.md", "r") as fh:
    long_description = fh.read()

setup(
    name="binaryreader",
    description="a c-extension that implements a binary reader on top of bytes and bytearrays",
    author="K0lb3",
    version="0.1.1",
    keywords=["python", "cpython", "c-api", "binary", "parsing"],
    classifiers=[
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
        "Intended Audience :: Developers",
        "Programming Language :: Python",
        "Programming Language :: Python :: 3",
    ],
    url="https://github.com/K0lb3/binaryreader",
    download_url="https://github.com/K0lb3/binaryreader/tarball/master",
    long_description=long_description,
    long_description_content_type="text/markdown",
    ext_modules=[
        Extension(
            "binaryreader",
            ["binaryreader.c",],
            language="c",
            extra_compile_args=["-std=c11"], # most compilers already use -03 or -02
        )
    ],
)
