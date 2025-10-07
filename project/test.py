#!/usr/bin/env python3
# filepath: run_tests.py
"""
Script para ejecutar automáticamente las 4 funcionalidades principales del procesador de imágenes.
Ejecuta: Ajuste de brillo, Convolución Gaussiana, Sobel y Rotación de imagen.
"""

import subprocess
import os
import time
from pathlib import Path

# Configuración
EXECUTABLE = "./img_processor"
IMAGE_PATH = "/home/samargo/Documents/universidad_/OS/second_assessment/project_andres/second_mitderm_operating_systems/project/src/img.jpeg"
RESULTS_DIR = "results"

# Crear directorio de resultados si no existe
Path(RESULTS_DIR).mkdir(exist_ok=True)

def run_command(input_commands, test_name):
    """
    Ejecuta el programa con una secuencia de comandos de entrada.
    
    Args:
        input_commands: Lista de comandos a enviar al programa
        test_name: Nombre descriptivo de la prueba
    """
    print(f"\n{'='*60}")
    print(f"EJECUTANDO: {test_name}")
    print(f"{'='*60}\n")
    
    # Unir comandos con saltos de línea
    input_data = "\n".join(input_commands) + "\n"
    
    try:
        # Ejecutar el programa
        process = subprocess.Popen(
            [EXECUTABLE, IMAGE_PATH],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        
        # Enviar comandos y capturar salida
        stdout, stderr = process.communicate(input=input_data, timeout=60)
        
        # Mostrar salida
        print(stdout)
        if stderr:
            print(f"ERRORES:\n{stderr}")
        
        print(f"\n✓ {test_name} completado\n")
        time.sleep(1)  # Pausa entre ejecuciones
        
    except subprocess.TimeoutExpired:
        process.kill()
        print(f"✗ TIMEOUT: {test_name} excedió 60 segundos")
    except Exception as e:
        print(f"✗ ERROR en {test_name}: {str(e)}")


def main():
    print("""
╔══════════════════════════════════════════════════════════╗
║     EJECUCIÓN AUTOMÁTICA DE FUNCIONALIDADES             ║
║     Procesador de Imágenes Multihilo                    ║
╚══════════════════════════════════════════════════════════╝
    """)
    
    # Verificar que el ejecutable existe
    if not os.path.exists(EXECUTABLE):
        print(f"✗ ERROR: No se encuentra {EXECUTABLE}")
        print("Ejecuta 'make' primero para compilar el proyecto")
        return
    
    # Verificar que la imagen de prueba existe
    if not os.path.exists(IMAGE_PATH):
        print(f"✗ ERROR: No se encuentra la imagen {IMAGE_PATH}")
        return
    
    print(f"Imagen de prueba: {IMAGE_PATH}")
    print(f"Directorio de resultados: {RESULTS_DIR}/")
    print()
    
    # ========================================================================
    # TEST 1: AJUSTE DE BRILLO
    # ========================================================================
    run_command(
        input_commands=[
            "9",                    # Configurar hilos
            "4",                    # 4 hilos
            "4",                    # Ajustar brillo
            "50",                   # Delta: +50
            "3",                    # Guardar imagen
            "test1_brightness.png", # Nombre archivo
            "11"                    # Salir
        ],
        test_name="TEST 1: Ajuste de Brillo (+50) con 4 hilos"
    )
    
    # ========================================================================
    # TEST 2: CONVOLUCIÓN GAUSSIANA (BLUR)
    # ========================================================================
    run_command(
        input_commands=[
            "9",                    # Configurar hilos
            "4",                    # 4 hilos
            "5",                    # Convolución Gaussiana
            "5",                    # Kernel 5x5
            "1.5",                  # Sigma: 1.5
            "3",                    # Guardar imagen
            "test2_gaussian.png",   # Nombre archivo
            "11"                    # Salir
        ],
        test_name="TEST 2: Convolución Gaussiana (5x5, σ=1.5) con 4 hilos"
    )
    
    # ========================================================================
    # TEST 3: DETECTOR DE BORDES SOBEL
    # ========================================================================
    run_command(
        input_commands=[
            "9",                    # Configurar hilos
            "4",                    # 4 hilos
            "6",                    # Aplicar Sobel
            "3",                    # Guardar imagen
            "test3_sobel.png",      # Nombre archivo
            "11"                    # Salir
        ],
        test_name="TEST 3: Detector de Bordes Sobel con 4 hilos"
    )
    
    # ========================================================================
    # TEST 4: ROTACIÓN DE IMAGEN
    # ========================================================================
    run_command(
        input_commands=[
            "9",                    # Configurar hilos
            "4",                    # 4 hilos
            "7",                    # Rotar imagen
            "90",                   # Ángulo: 90 grados
            "3",                    # Guardar imagen
            "test4_rotation.png",   # Nombre archivo
            "11"                    # Salir
        ],
        test_name="TEST 4: Rotación de Imagen (90°) con 4 hilos"
    )
    
    # ========================================================================
    # RESUMEN FINAL
    # ========================================================================
    print("\n" + "="*60)
    print("TODAS LAS PRUEBAS COMPLETADAS")
    print("="*60)
    print(f"\nResultados guardados en: {RESULTS_DIR}/")
    print("\nArchivos generados:")
    print("  • test1_brightness.png  - Brillo ajustado (+50)")
    print("  • test2_gaussian.png    - Blur Gaussiano (5x5, σ=1.5)")
    print("  • test3_sobel.png       - Detección de bordes")
    print("  • test4_rotation.png    - Rotación 45 grados")
    print()


if __name__ == "__main__":
    main()