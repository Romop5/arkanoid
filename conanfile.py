from conan import ConanFile
from conan.tools.cmake import cmake_layout


class ArkanoidRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        self.requires("sdl/2.30.1")

    def layout(self):
        cmake_layout(self)