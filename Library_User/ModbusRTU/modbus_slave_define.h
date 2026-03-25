#ifndef _MODBUS_SLAVER_DEFINE_H_
#define _MODBUS_SLAVER_DEFINE_H_

// =======================  Input registers (3X) ================== //
// Chỉ đọc
#define ADR_INPUT_SIGNAL_QUALITY          0    // u16, chất lượng sóng (3x0)
#define ADR_INPUT_BMS_STATUS              1    // bit, trạng thái BMS (3x1)
#define ADR_INPUT_SOLAR_FAULT            10    // u16, lỗi Solar (3x10)

// Nhóm pin / cell
#define ADR_INPUT_PACK_VOLTAGE           32    // float, điện áp pin (3x32) - 2 thanh ghi
#define ADR_INPUT_PACK_CURRENT           33    // float, dòng điện pin (3x33) - 2 thanh ghi
#define ADR_INPUT_PACK_CAPACITY          34    // u16, dung lượng pin (3x34)
#define ADR_INPUT_CELL_TEMP              35    // u16, nhiệt độ cell (3x35)
#define ADR_INPUT_MOS_TEMP               36    // u16, nhiệt độ MOS (3x36)
#define ADR_INPUT_RUNTIME                37    // u16, thời gian chạy (3x37)

// Điện áp cell 1..30 (mỗi cell u16). Cell 1 bắt đầu tại 3x40.
#define ADR_INPUT_CELL_VOLT_BASE 40
#define ADR_INPUT_CELL31_VOLT 84
#define ADR_INPUT_CELL32_VOLT 85
#define ADR_INPUT_CELL_INVALID 0xFFFF
#define ADR_INPUT_CELL_VOLT(n) \
    (((n) >= 1 && (n) <= 30) ? (ADR_INPUT_CELL_VOLT_BASE + ((n) - 1)) : \
    ((n) == 31) ? ADR_INPUT_CELL31_VOLT : \
    ((n) == 32) ? ADR_INPUT_CELL32_VOLT : \
    ADR_INPUT_CELL_INVALID)

// Chỉ số cell max/min 
#define ADR_INPUT_CELL_MAX_INDEX         70    // 3x70
#define ADR_INPUT_CELL_MIN_INDEX         71    // 3x71
// Giá trị cell max/min
#define ADR_INPUT_CELL_MAX_VALUE         72    // 3x72
#define ADR_INPUT_CELL_MIN_VALUE         73    // 3x73
// Điện áp cell trung bình
#define ADR_INPUT_CELL_AVE_VALUE         74    // 3x74

// Cảnh báo BMS - Từng trường riêng lẻ (u16, mỗi giá trị 2 bit: 0=OK, 1=Warning, 2=Alarm, 3=Critical)
#define ADR_INPUT_BMS_ALM_CELL_OVERVOLT  75    // 3x75, Cell quá áp (bit 0-1)
#define ADR_INPUT_BMS_ALM_CELL_UNDERVOLT 76    // 3x76, Cell thấp áp (bit 2-3)
#define ADR_INPUT_BMS_ALM_DELTA_OVER     77    // 3x77, ΔV quá lớn (bit 8-9)
#define ADR_INPUT_BMS_ALM_DCHG_OC        78    // 3x78, Dòng xả quá (bit 10-11)
#define ADR_INPUT_BMS_ALM_CHG_OC         79    // 3x79, Dòng sạc quá (bit 12-13)
#define ADR_INPUT_BMS_ALM_TEMP_HIGH      80    // 3x80, Nhiệt độ cao (bit 14-15)
#define ADR_INPUT_BMS_ALM_TEMP_LOW       81    // 3x81, Nhiệt độ thấp (bit 16-17)
#define ADR_INPUT_BMS_ALM_SOC_LOW        82    // 3x82, SOC thấp (bit 20-21)
#define ADR_INPUT_BMS_ALM_COMM_FAULT     83    // 3x83, Lỗi truyền thông (bit 28-29)

#define ADR_INPUT_GPS_ACTIVE             86   // bit, 1 = GPS active/fix, 0 = chưa active (3x84)
#define ADR_INPUT_IMD_POS_RISO_KOHM      87   // u16, Riso POS (kOhm)
#define ADR_INPUT_IMD_NEG_RISO_KOHM      88   // u16, Riso NEG (kOhm)

// Nhóm Driver / động cơ
#define ADR_INPUT_DRIVER_VOLT            102   // u16, điện áp driver (3x102)
#define ADR_INPUT_DRIVER_CURRENT         103   // u16, dòng điện driver (3x103)
#define ADR_INPUT_BALANCE                104   // u16, cân số (3x104)
#define ADR_INPUT_SPEED                  105   // u16, tốc độ (3x105)
#define ADR_INPUT_MOTOR_TEMP             106   // u16, nhiệt độ motor (3x106)
#define ADR_INPUT_DRIVER_TEMP            107   // u16, nhiệt độ Driver (3x107)
#define ADR_INPUT_VELOCITY               108   // u16, vận tốc (3x108)
#define ADR_INPUT_DRIVER_FAULT           109   // u16, trạng thái motor: 0=OK, 1=warning, 2=fault (3x109)

#define ADR_INPUT_EVENT_POWER_STATUS     110   // u16, sự kiện thay đổi trạng thái công suất (3x110)
#define ADR_INPUT_EVENT_GEAR_STATUS		 111   // u16, sự kiện thay đổi trạng thái cần số (3x111)
#define ADR_INPUT_EVENT_SUDDEN_ACCELERATION 112   // u16, sự kiện tăng tốc đột ngột (3x112)
#define ADR_INPUT_EVENT_MOTOR_STATUS     113   // u16, sự kiện thay đổi trạng thái động cơ (3x113)
#define ADR_INPUT_EVENT_GPS_STATUS		 114   // u16, sự kiện thay đổi trạng thái GPS (3x114)
#define ADR_INPUT_EVENT_VCU_STATUS 		 115   // u16, sự kiện thay đổi trạng thái hệ thống (3x115)
#define ADR_INPUT_DRIVER_THR_STATUS      116   // u16, trạng thái validate/save ngưỡng lỗi driver (3x116)
#define ADR_INPUT_DRIVER_ALM_TEMP_HIGH   120   // u16, cảnh báo nhiệt controller cao (3x120)
#define ADR_INPUT_DRIVER_ALM_MOTOR_TEMP  121   // u16, cảnh báo nhiệt motor cao (3x121)
#define ADR_INPUT_DRIVER_ALM_UNDER_VOLT  122   // u16, cảnh báo thấp áp (3x122)
#define ADR_INPUT_DRIVER_ALM_OVER_VOLT   123   // u16, cảnh báo quá áp (3x123)
#define ADR_INPUT_IMD_MEASURE_WARN       124   // u16, cảnh báo lỗi CAN driver (3x124)


// =======================  Holding registers (4X) ================== //
#define ADR_HOLD_DRIVER_TEMP_HIGH_C      150   // 4x150, ngưỡng nhiệt controller cao
#define ADR_HOLD_MOTOR_TEMP_HIGH_C       151   // 4x151, ngưỡng nhiệt motor cao
#define ADR_HOLD_DRIVER_UNDER_VOLT_V     152   // 4x152, ngưỡng thấp áp
#define ADR_HOLD_DRIVER_OVER_VOLT_V      153   // 4x153, ngưỡng quá áp
#define ADR_HOLD_DRIVER_ALM_TEMP_HIGH_C  154   // 4x154, ngưỡng cảnh báo nhiệt controller cao
#define ADR_HOLD_DRIVER_ALM_MOTOR_TEMP_C 155   // 4x155, ngưỡng cảnh báo nhiệt motor cao
#define ADR_HOLD_DRIVER_ALM_UNDER_VOLT_V 156   // 4x156, ngưỡng cảnh báo thấp áp
#define ADR_HOLD_DRIVER_ALM_OVER_VOLT_V  157   // 4x157, ngưỡng cảnh báo quá áp
#define ADR_HOLD_DRIVER_THR_SAVE         160   // 4x160, ghi 1 để lưu EPROM

// =======================  Coils (0X) ================== //
// Có thể đọc/ghi 
#define ADR_COIL_LOGO_AC                 0     // Logo điều hòa  (0x0)
#define ADR_COIL_LOGO_LIGHT              1     // Logo Đèn       (0x1)
#define ADR_COIL_LOGO_SOLAR              2     // Logo NLMT      (0x2)
#define ADR_COIL_LOGO_BATTERY            3     // Logo PIN       (0x3)
#define ADR_COIL_LOGO_MOTOR              4     // Logo động cơ  (0x4)
#define ADR_COIL_LOGO_CHARGE             5     // Logo Sạc      (0x5)
#define ADR_COIL_MOS_CHARGE              6     // MOS sạc       (0x6)
#define ADR_COIL_MOS_DISCHARGE           7     // MOS xả        (0x7)
#define ADR_COIL_MOS_BALANCE             8     // MOS cân bằng  (0x8)
#define ADR_COIL_MOTOR_STATE         	 9     // Trạng thái đóng cơ (0x9)

// Lỗi BMS - Từng trường riêng lẻ (coil, mỗi giá trị 1 bit: 0=OK, 1=Lỗi)
#define ADR_COIL_BMS_ERR_LINE_RES_HIGH     10    // 0x10, Điện trở dây cao (bit 0)
#define ADR_COIL_BMS_ERR_MOS_OVERTEMP      11    // 0x11, MOS quá nhiệt (bit 1)
#define ADR_COIL_BMS_ERR_CELL_COUNT_MISMATCH 12  // 0x12, Số lượng cell không khớp (bit 2)
#define ADR_COIL_BMS_ERR_CUR_SENSOR_FAULT  13    // 0x13, Lỗi cảm biến dòng (bit 3)
#define ADR_COIL_BMS_ERR_CELL_OVERVOLT     14    // 0x14, Cell quá áp (bit 4)
#define ADR_COIL_BMS_ERR_PACK_OVERVOLT     15    // 0x15, Tổng áp pack cao (bit 5)
#define ADR_COIL_BMS_ERR_CHG_OVERCURRENT   16    // 0x16, Dòng sạc quá mức (bit 6)
#define ADR_COIL_BMS_ERR_CHG_SHORT         17    // 0x17, Ngắn mạch khi sạc (bit 7)
#define ADR_COIL_BMS_ERR_CHG_TEMP_HIGH     18    // 0x18, Nhiệt sạc cao (bit 8)
#define ADR_COIL_BMS_ERR_CHG_TEMP_LOW      19    // 0x19, Nhiệt sạc thấp (bit 9)
#define ADR_COIL_BMS_ERR_COMM_INNER_FAULT  20    // 0x20, Lỗi truyền thông nội bộ (bit 10)
#define ADR_COIL_BMS_ERR_CELL_UNDERVOLT    21    // 0x21, Cell thấp áp (bit 11)
#define ADR_COIL_BMS_ERR_PACK_UNDERVOLT    22    // 0x22, Pack thấp áp (bit 12)
#define ADR_COIL_BMS_ERR_DCHG_OVERCURRENT  23    // 0x23, Dòng xả quá mức (bit 13)
#define ADR_COIL_BMS_ERR_DCHG_SHORT        24    // 0x24, Ngắn mạch khi xả (bit 14)
#define ADR_COIL_BMS_ERR_DCHG_TEMP_HIGH    25    // 0x25, Nhiệt xả cao (bit 15)
#define ADR_COIL_BMS_ERR_CHG_MOS_FAULT     26    // 0x26, Lỗi MOS sạc (bit 16)
#define ADR_COIL_BMS_ERR_DCHG_MOS_FAULT    27    // 0x27, Lỗi MOS xả (bit 17)



// Lỗi Driver - Từng trường riêng lẻ (coil, mỗi giá trị 1 bit: 0=OK, 1=Lỗi)
#define ADR_COIL_DRIVER_OVER_CURRENT       28    // 0x28, Driver: Quá dòng
#define ADR_COIL_DRIVER_TEMP_HIGH          29    // 0x29, Driver: Nhiệt độ bộ ĐK cao
#define ADR_COIL_DRIVER_ENCODER_ERROR      30    // 0x30, Driver: Lỗi bộ mã hóa động cơ
#define ADR_COIL_DRIVER_CAN_COMM_ERROR     31    // 0x31, Driver: Lỗi truyền thông CAN
#define ADR_COIL_DRIVER_UNDER_VOLT         32    // 0x32, Driver: Điện áp quá thấp
#define ADR_COIL_DRIVER_OVER_VOLT          33    // 0x33, Driver: Điện áp quá cao
#define ADR_COIL_DRIVER_MOTOR_TEMP_HIGH    34    // 0x34, Driver: Nhiệt độ động cơ cao
#define ADR_COIL_DRIVER_MOTOR_TEMP_SENSOR  35    // 0x35, Driver: Lỗi cảm biến nhiệt độ động cơ
#define ADR_COIL_DRIVER_ACCEL_FAULT        36    // 0x36, Driver: Lỗi chân ga
#define ADR_COIL_DRIVER_ACCEL_SAFETY_INTERLOCK 37 // 0x37, Driver: Interlock nhả ga
#define ADR_COIL_DRIVER_ACCELERATE_CHARGE    38    // 0x38, Driver: Tăng tốc khi khởi tạo
#define ADR_COIL_DRIVER_ACCELERATE_ERROR  39    // 0x39, Driver: Tăng tốc khi sạc
#define ADR_COIL_DRIVER_ACCELERATE_INIT   40    // 0x40, Driver: Tăng tốc khi lỗi

// Lỗi IMD
#define ADR_COIL_IMD_FAULT_BOTH          60   // u16, lỗi IMD kép (0=OK,1=Fault)

// Lỗi hệ thống
#define ADR_COIL_SYSTEM_CAN_DRIVER       80
#define ADR_COIL_IMD_FAULT_ANY           81   // u16, lỗi IMD (0=OK,1=Fault)

#endif /* _MODBUS_SLAVER_DEFINE_H_ */
