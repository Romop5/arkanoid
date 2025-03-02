from conan import ConanFile
from conan.tools.cmake import cmake_layout


class ArkanoidRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def configure(self):
         self.options["sdl_image"].with_libjpeg = False
         self.options["sdl_image"].with_libtiff = False
         self.options["sdl_image"].with_libwebp = False
         
    def requirements(self):
        self.requires("sdl/2.30.1", override=True)
        self.requires("sdl_image/2.0.5")
    
    def layout(self):
        cmake_layout(self)