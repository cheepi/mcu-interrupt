# MCU Interrupt Project - STM32
Project ini dibuat untuk demonstrasi penggunaan interrupt eksternal (pada tombol) untuk mengubah pola LED menggunakan STM32 dan HAL library.
Project ini juga merupakan implementasi dari **Tugas Hands On 2 Mata Kuliah Sistem Berbasis Mikroprosesor**, dengan requirement:
- Minimum 3 variasi LED (termasuk default)
- Pemilihan variasi menggunakan push button + interrupt
- Dilengkapi dengan debouncing


## Perangkat & Tools
- MCU: **STM32F401CCU6** (Black Pill)
- IDE: **STM32CubeIDE**
- Library: **HAL (Hardware Abstraction Layer)**
- Button input: **PA0 (EXTI0, rising edge)**
- LED Output:
  1. **PA5 (LED1)**
  2. **PA6 (LED2)**
  3. **PC13 (LED3, built-in, active-low)**
      

## LED Pattern Summary
Tombol ditekan → interrupt terjadi → pola berubah (circular: 0 → 1 → 2 → 3 → 0 ...)

| Pola | Deskripsi |
|------|-----------|
| 0    | Semua LED menyala |
| 1    | Semua LED blinking bersamaan tiap 500ms |
| 2    | LED nyala beda rasio (3:2:1) selama 1200ms total (PA5:500ms, PA6:300ms, PC13:1100ms) |
| 3    | LED nyala satu per satu secara bergantian tiap 500ms |

Detail lengkap bisa dilihat di dokumen:  
[Pattern Description (PDF)](Docs/pattern-desc.pdf)


## Interrupt Logic
- Tombol pada **PA0** di-set sebagai **EXTI (Interrupt Rising Edge)**
- Callback `HAL_GPIO_EXTI_Callback()` digunakan untuk mendeteksi penekanan tombol
- Implementasi **debouncing software 200ms** dilakukan dengan `HAL_GetTick()` (delay berbasis waktu)


## Dokumentasi Rangkaian Fisik
[Rangkaian Fisik](Docs/setup.jpg)

[Video Jalannya Program - Youtube](https://www.youtube.com/watch?v=N6ts76qWAWY)
