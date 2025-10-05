# Multithreaded Image Processing System

A high-performance, multithreaded image processing framework implemented in C with POSIX threads. Features concurrent execution of various image filters and operations with configurable parallelization for optimal performance.

## Table of Contents

- [Features](#features)
- [Architecture](#architecture)
- [Prerequisites](#prerequisites)
- [Installation](#installation)
- [Usage](#usage)
- [Modules](#modules)
- [Performance](#performance)
- [Technical Details](#technical-details)
- [Project Structure](#project-structure)


## Features

### Core Capabilities

- **PNG Image I/O**: Load and save PNG images (grayscale and RGB) using the stb_image library
- **Concurrent Processing**: POSIX thread-based parallelization with configurable thread count (1-16 threads)
- **Real-time Performance Monitoring**: Wall-clock time measurements for accurate parallelization benchmarking
- **Multiple Image Filters**:
  - Brightness adjustment with concurrent execution
  - Gaussian blur via convolution filtering
  - Sobel edge detection with gradient magnitude calculation
  - **Concurrent image rotation with bilinear interpolation**
- **Automated Benchmarking**: Compare performance across different thread configurations
- **Interactive CLI**: User-friendly menu-driven interface with comprehensive options

### Advanced Features

- Modular architecture with clean separation of concerns
- Robust error handling and memory management
- Automatic RGB to grayscale conversion for edge detection
- Configurable Gaussian kernel generation with custom sigma values
- Thread-safe image operations with proper synchronization
- **Geometric transformations with subpixel accuracy and anti-aliasing**

## Architecture

The system follows a modular design pattern with specialized components:

```
┌─────────────────────────────────────────────────┐
│              Main Application                   │
│           (Interactive Menu Loop)               │
└────────────────┬────────────────────────────────┘
                 │
    ┌────────────┴────────────┐
    │                         │
┌───▼────────┐       ┌────────▼──────┐
│  Image I/O │       │   Processing  │
│   Module   │       │     Engine    │
└────────────┘       └───────┬───────┘
                             │
              ┌──────────────┼──────────────┬──────────────┐
              │              │              │              │
         ┌────▼───┐    ┌─────▼────┐   ┌────▼────┐   ┌─────▼─────┐
         │ Filters│    │Threading │   │Benchmark│   │ Rotation  │
         │ Module │    │  Manager │   │ Module  │   │  Module   │
         └────────┘    └──────────┘   └─────────┘   └───────────┘
```

## Prerequisites

### System Requirements

- **Operating System**: Linux (tested on Ubuntu 20.04+, Kali Linux)
- **Compiler**: GCC 7.0+ with C99 support
- **Libraries**:
  - `pthread` (POSIX threads)
  - `libm` (math library)
- **Build Tools**: GNU Make 4.0+

### Dependencies

The project uses the following header-only libraries (included in the `stb/` directory):

- [stb_image.h](https://github.com/nothings/stb/blob/master/stb_image.h) - Image loading
- [stb_image_write.h](https://github.com/nothings/stb/blob/master/stb_image_write.h) - Image writing

## Installation

### 1. Clone the Repository

```bash
git clone <repository-url>
cd v1.0/project
```

### 2. Download STB Libraries (if not included)

```bash
mkdir -p stb
cd stb
wget https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
wget https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h
cd ..
```

### 3. Build the Project

```bash
make
```

This will:
- Compile all source files with optimization flags (`-O2`)
- Create object files in the `obj/` directory
- Generate the executable `img_processor`

### Build Options

```bash
make clean      # Remove compiled objects and executable
make clean-all  # Remove all generated files including results
make rebuild    # Clean and rebuild from scratch
make run        # Build and run the interactive program
make help       # Display available make targets
```

## Usage

### Basic Execution

```bash
./img_processor [optional_image_path.png]
```

### Interactive Menu

Upon launch, you'll see the following options:

```
╔══════════════════════════════════════════════════════╗
║   Plataforma de Edición de Imágenes - Linux C      ║
╚══════════════════════════════════════════════════════╝
  0. Benchmark de paralelización (prueba automática)
  1. Cargar imagen PNG
  2. Mostrar matriz de píxeles
  3. Guardar como PNG
  4. Ajustar brillo (+/- valor) concurrentemente
  5. Aplicar convolución Gaussiana (blur)
  6. Aplicar detector de bordes Sobel
  7. Rotar imagen (grados)
  8. Escalar Imagen
  9. Configurar número de hilos (actual: 4)
  10. Información del sistema
  11. Salir
```

### Example Workflow

1. **Load an image**: Select option `1` and provide the path to a PNG file
2. **Configure threads**: Select option `8` to set thread count (e.g., 4 or 8)
3. **Apply filters**:
   - Brightness: Option `4` (e.g., +50 to brighten, -30 to darken)
   - Gaussian blur: Option `5` (kernel size: 5, sigma: 1.0)
   - Edge detection: Option `6`
   - **Image rotation**: Option `7` (e.g., 45 degrees for diagonal rotation)
4. **Save result**: Option `3` (saves to `results/` directory)
5. **Run benchmark**: Option `0` to test performance with 1, 2, 4, 8 threads

### Sample Commands

```bash
# Run with a test image
./img_processor shark.png

# Build and run interactively
make run
```

## Modules

### Core Modules

#### 1. `image.c/h` - Image Data Management
- Defines the `ImagenInfo` structure for storing image metadata and pixel data
- Provides memory management functions (`liberarImagen`, `imagenCargada`)
- Handles RGB to grayscale conversion with perceptual weighting

#### 2. `image_io.c/h` - I/O Operations
- Loads PNG files using stb_image with automatic format detection
- Saves processed images to PNG format
- Implements pixel matrix visualization for debugging

#### 3. `filters.c` - Image Processing Algorithms
- **Brightness Adjustment**: Concurrent pixel-wise addition with clamping
- **Gaussian Convolution**: Separable kernel generation and edge-aware convolution
- **Sobel Operator**: Combined Gx/Gy gradient computation with magnitude calculation

#### 4. `threading.c/h` - Concurrency Management
- Thread pool creation and workload distribution
- Wall-clock time measurement using `gettimeofday`
- Global thread configuration with validation

#### 5. `benchmark.c/h` - Performance Analysis
- Automated testing with multiple thread counts (1, 2, 4, 8)
- Comparative performance metrics and speedup calculations
- System information display

#### 6. `image_rotation.c/h` - Concurrent Image Rotation Module

**Implemented in** [`image_rotation.c`](project/src/image_rotation.c) **and** [`image_rotation.h`](project/include/image_rotation.h)

This module provides high-performance concurrent image rotation with the following capabilities:

- **Parallel Execution**: Utilizes POSIX threads (pthreads) to distribute rotation computation across multiple CPU cores
- **Bilinear Interpolation**: Implements subpixel-accurate sampling for smooth, anti-aliased rotation results
- **Inverse Transformation**: Applies inverse geometric rotation matrices to map destination pixels back to source coordinates
- **Multi-Channel Support**: Processes both grayscale (1-channel) and RGB (3-channel) images seamlessly
- **Automatic Bounding Box Computation**: Calculates optimal output dimensions to prevent clipping of rotated content
- **Scalable Architecture**: Divides workload by rows among threads with minimal synchronization overhead

**Key Functions:**
- `rotateImageConcurrent()`: Main entry point that accepts an image structure and rotation angle in degrees, producing a rotated output image with dynamically allocated memory

**Technical Implementation:**
- Rotation matrix computation: `cos(θ)`, `sin(θ)` transformations with angle conversion to radians
- Per-thread workload assignment based on output image height
- Bilinear weight calculation for four neighboring source pixels
- Channel-independent interpolation for RGB images
- Wall-clock timing for performance measurement

**Usage Pattern:**
The rotation function is invoked through the interactive menu (option 7) where the user specifies a rotation angle. The function:
1. Validates the input image
2. Computes new dimensions based on rotation angle
3. Allocates memory for the rotated output
4. Distributes rows among threads for concurrent processing
5. Each thread performs inverse mapping with bilinear interpolation
6. Replaces the original image with the rotated result
7. Reports execution time and configuration details

**Extension Potential:**
The concurrent rotation implementation serves as a reference architecture for developing additional geometric transformation operations such as:
- Affine transformations (scaling, shearing, translation)
- Perspective transformations
- Image warping and distortion effects
- Multi-stage transformation pipelines

All operations follow the same parallelization pattern: inverse coordinate mapping combined with interpolation, enabling consistent performance scaling across multi-core systems.

## Performance

### Benchmark Results

Typical performance improvements on a 4-core system (example with 1920x1080 image):

| Threads | Brightness (s) | Gaussian Blur (s) | Sobel (s) | Speedup |
|---------|---------------|-------------------|-----------|---------|
| 1       | 0.042         | 1.234             | 0.891     | 1.00x   |
| 2       | 0.023         | 0.645             | 0.467     | 1.92x   |
| 4       | 0.013         | 0.338             | 0.245     | 3.65x   |
| 8       | 0.011         | 0.289             | 0.209     | 4.26x   |

**Note:** Image rotation performance scales similarly to Gaussian blur due to comparable computational complexity (interpolation vs. convolution). Expect 3-4x speedup on quad-core systems for rotation operations.

### Optimization Strategies

- **Row-based partitioning**: Each thread processes contiguous rows for cache efficiency
- **Wall-clock timing**: Accurately measures parallelization benefits (not CPU time)
- **Minimal synchronization**: Lock-free pixel access with independent workloads
- **Compiler optimizations**: `-O2` flag for performance without sacrificing debugging
- **Inverse mapping**: Eliminates gaps in output by sampling source coordinates directly

## Technical Details

### Thread Synchronization

- Uses POSIX threads (`pthread_create`, `pthread_join`)
- No mutexes required due to non-overlapping memory regions per thread
- Thread arguments passed via dedicated structures to avoid race conditions

### Memory Model

```c
typedef struct {
    int ancho;               // Width in pixels
    int alto;                // Height in pixels
    int canales;             // Channels: 1 (grayscale) or 3 (RGB)
    unsigned char*** pixeles; // 3D array: [height][width][channels]
} ImagenInfo;
```

- Dynamic allocation for flexible image sizes
- Proper cleanup to prevent memory leaks
- RGB stored as contiguous channel values per pixel

### Algorithm Details

#### Gaussian Blur
- Kernel generation: `G(x,y) = (1/2πσ²) * exp(-(x²+y²)/2σ²)`
- Normalization ensures energy preservation
- Edge handling: clamps indices to valid ranges

#### Sobel Edge Detection
1. Convert to grayscale using ITU-R BT.601 coefficients
2. Apply horizontal (Gx) and vertical (Gy) kernels:
   ```
   Gx = [-1  0  1]    Gy = [-1 -2 -1]
        [-2  0  2]         [ 0  0  0]
        [-1  0  1]         [ 1  2  1]
   ```
3. Compute gradient magnitude: `√(Gx² + Gy²)`
4. Normalize to 0-255 range

#### Concurrent Image Rotation
1. **Angle Conversion**: Convert degrees to radians (θ_rad = θ_deg × π/180)
2. **Bounding Box Calculation**: Compute rotated corner coordinates to determine output dimensions:
   - Transform all four corners using rotation matrix
   - Find min/max x and y coordinates
   - Allocate output image with new dimensions
3. **Rotation Matrix Application**:
   ```
   R(θ) = [ cos(θ)  -sin(θ) ]
          [ sin(θ)   cos(θ) ]
   ```
4. **Inverse Mapping**: For each destination pixel (x', y'):
   - Translate to origin-centered coordinates
   - Apply inverse rotation matrix
   - Translate back to source image space
5. **Bilinear Interpolation**:
   - Identify four neighboring source pixels
   - Compute fractional offsets (dx, dy)
   - Calculate weights: w1=(1-dx)(1-dy), w2=dx(1-dy), w3=(1-dx)dy, w4=dx·dy
   - Interpolate each channel independently: `value = Σ(pixel_i × weight_i)`
6. **Boundary Handling**: Clamp source coordinates to valid image bounds

**Mathematical Formulation:**

For a rotation angle θ and destination pixel (x_d, y_d), the source coordinates (x_s, y_s) are computed as:

```
x_c = x_d - (new_width / 2)
y_c = y_d - (new_height / 2)

x_s = x_c × cos(-θ) - y_c × sin(-θ) + (original_width / 2)
y_s = x_c × sin(-θ) + y_c × cos(-θ) + (original_height / 2)

// Bilinear interpolation
x0 = floor(x_s), y0 = floor(y_s)
dx = x_s - x0, dy = y_s - y0

output(x_d, y_d) = (1-dx)(1-dy)·input(x0, y0) 
                 + dx(1-dy)·input(x0+1, y0)
                 + (1-dx)dy·input(x0, y0+1)
                 + dx·dy·input(x0+1, y0+1)
```

## Project Structure

```
project/
├── src/                    # Source files
│   ├── main.c             # Entry point and menu loop
│   ├── image.c            # Image structure management
│   ├── image_io.c         # PNG load/save operations
│   ├── brightness.c       # Brightness adjustment filter
│   ├── convolution.c      # Gaussian blur implementation
│   ├── sobel.c            # Sobel edge detection
│   ├── image_rotation.c   # Image rotation functionality
│   ├── threading.c        # Threading utilities
│   ├── benchmark.c        # Performance testing
    └── scaling.c          # Resize image
├── include/               # Header files
│   ├── image_rotation.h
│   ├── image.h
│   ├── image_io.h
│   ├── filters.h
│   ├── threading.h
│   ├── benchmark.h
    └── scaling.h
├── stb/                   # Third-party libraries
│   ├── stb_image.h
│   └── stb_image_write.h
├── results/               # Output directory for processed images
├── obj/                   # Compiled object files (generated)
├── Makefile              # Build configuration
└── shark.png             # Sample test image
```

## Acknowledgments

- [stb libraries](https://github.com/nothings/stb) by Sean Barrett for image I/O
- POSIX threads documentation and community examples
- Academic resources on parallel computing and image processing algorithms


---

**Built with ❤️ for Operating Systems coursework - Semester 7**