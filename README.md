# vulkandemo

This project is used to learn Vulkan from scratch. All codes are just "copy" from Vulkan official turtorial. The code is a learning hint for the further mastering vulkan. If there is any new experience about vulkan, I will write down here as soon as possible.

As far as I know, there are some difficulties used on macOS via MoltenVK.

* MoltenVK does not support lunarG standard vailidation layers.

* GLFW 3.3 or newer is required.

* The vendor must support VK_MVK_macos_surface extension to create display surface.

* The feature device geomery shader is not supported on my macbook, but tessllation shader is supported. I don't know whether my macbook is old(2013 Late with NVIDIA GT 750M) or others。
