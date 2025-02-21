# Printer JSON Format Qo'llanmasi

Quyidagi JSON formatida printer uchun hujjat yaratish mumkin.

## JSON Strukturasi

```json
[
    {
        "type": "text",                  // text, qrCode
        "align": "center",               // left, center, right (Default: left)
        "body": "Tashkent City Parking", // Asosiy qismi, type=text bo'lsa matn, type=qrCode bo'lsa QR kod ma'lumoti
        "font": "normal"                 // normal, bold, large, underline (Default: normal)
    }
]
```

### Xususiyatlar:
- **type**: `text` yoki `qrCode` qiymatlarini oladi.
- **align**: `left`, `center`, `right`. (Default: `left`)
- **body**: `text` bo'lsa matn, `qrCode` bo'lsa QR kod ma'lumoti.
- **font**: `normal`, `bold`, `large`, `underline`. (Default: `normal`)

## Matn sig‘imi
- **Normal font**: 1 qatorda **42 ta belgi** sig‘adi.
- **Bold yoki Large font**: 1 qatorda **21 ta belgi** sig‘adi.

## Xat boshi tashlash (`\n\r`)
Xat boshi tashlash uchun `\n\r` ishlatiladi:

```json
{
    "type": "text",
    "align": "center",
    "body": "\n\r",
    "font": "normal"
}
```

### Bir nechta xat boshi tashlash
Ikkita yoki undan ortiq xat boshi tashlash uchun har bir qator uchun alohida element yaratish shart emas. Quyidagi kod ishlaydi:

```json
{
    "type": "text",
    "align": "center",
    "body": "\n\r\n\r",
    "font": "normal"
}
```

> **Eslatma:** Bu qoida faqat ikkita emas, balki bir nechta xat boshi qo‘shish uchun ham amal qiladi.

