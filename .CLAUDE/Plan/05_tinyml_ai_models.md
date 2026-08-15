# Task 5: TinyML AI Model Training & Exporter Python Scripts

## Mục tiêu
Tạo mã nguồn Python huấn luyện 2 mô hình AI từ Kaggle Datasets và tự động xuất thành file C++ INT8 lượng tử hóa nhúng vào ESP32-S3.

## Các File Cần Tạo / Sửa
- `ai_models/dataset_downloader.py`
- `ai_models/train_fall_model.py`
- `ai_models/train_arrhythmia_model.py`
- `firmware/include/fall_model_data.h`
- `firmware/include/arrhythmia_model_data.h`

## Các Bước Triển Khai
- [ ] **Bước 1**: Viết `dataset_downloader.py` tự động tải Kaggle Dataset 1 và Dataset 3.
- [ ] **Bước 2**: Viết `train_fall_model.py` huấn luyện Random Forest INT8 (~8KB) từ Fall Dataset.
- [ ] **Bước 3**: Viết `train_arrhythmia_model.py` huấn luyện 1D-CNN INT8 (~30KB) từ MIT-BIH Dataset.
- [ ] **Bước 4**: Xuất 2 file header C++ chứa trọng số mô hình INT8 vào `firmware/include/`.
