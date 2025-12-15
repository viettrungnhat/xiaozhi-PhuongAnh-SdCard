from setuptools import setup, find_packages

setup(
    name="xiaozhi-phuonganh-sdcard",
    version="0.1.0",
    description="Virtual assistant with SD card support for Phuong Anh",
    author="Viet Trung Nhat",
    packages=find_packages(where="src"),
    package_dir={"": "src"},
    python_requires=">=3.7",
    install_requires=[
        # All dependencies are part of Python standard library
    ],
    entry_points={
        "console_scripts": [
            "xiaozhi=xiaozhi.main:main",
        ],
    },
)
