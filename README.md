# Control de Polarización Automático (OPTA Positioner)

Prototipo de desarrollo para el posicionamiento motorizado de paletas de polarización en un láser de fibra dopada con erbio (EDFL).

> **ESTADO DEL PROYECTO: EN DESARROLLO (Fase Inicial / Hardware Base)**  
> El sistema se encuentra en etapa de pruebas mecánicas y de actuación. La integración del sensor de posición y el bucle cerrado de control están pendientes de implementación.

---

## 🛠️ Estado Actual del Hardware y Firmware

| Componente           | Estado / Configuración       | Notas                                                        |
| :------------------- | :--------------------------- | :----------------------------------------------------------- |
| **Controlador**      | ESP32-S3                     | En comunicación por Bluetooth / Puerto Serie                 |
| **Driver de Motor**  | Makerbase MKS TMC2160-OC     | Configurado para control por pulso/dirección (`STEP`/`DIR`)  |
| **Motor**            | NEMA 17 (17HS8401 / MOT0007) | Funcionamiento en lazo abierto                               |
| **Encoder (AS5600)** | Pendiente de integración     | Físicamente contemplado, pendiente de desarrollo en firmware |

---

## 📂 Estructura del Repositorio

```text
.
├── 3D Modeling/           # Diseños CAD (.FCStd), piezas en STEP, STL y planos (PDF/SVG)
│   ├── Acople Iman/       # Piezas para montaje del imán del encoder
│   └── Control Polarizacion/
│       ├── CAD/           # Piezas del ensamble (Eje, Paleta, Corona, etc.)
│       ├── Planos/        # Documentación técnica y vistas acotadas
│       ├── STEP/          # Archivos intercambiables
│       └── STL/           # Archivos para impresión 3D
├── Motor/                 # Firmware preliminar para pruebas de actuador
│   ├── MovimientoBT/      # Script de prueba básica de movimiento vía Bluetooth
└── README.md
```

---

## 📋 Hoja de Ruta (Roadmap)

1. [x] Diseño CAD preliminar y planos de fabricación de las piezas mecánicas.
2. [x] Pruebas iniciales de movimiento del motor NEMA 17 con el driver TMC2160.
3. [ ] Integración I2C del sensor magnético **AS5600** en el ESP32-S3 para lectura de ángulo real.
4. [ ] Aplicar lazo cerrado para control de posición (posible PID).
5. [ ] Diseño de aplicación vía Bluetooth para movimiento de los motores.
6. [ ] Extender implementación a los 3 motores.
7. [ ] Habilitar comunicación SPI para driver TMC2160
8. [ ] Implementar un patrón de toma de datos automatizado.    
9. [ ] Implementación de algoritmo de búsqueda de *mode-locking* en el láser mediante Python.
