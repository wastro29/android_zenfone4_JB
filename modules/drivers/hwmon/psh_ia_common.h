#ifndef _PSH_IA_COMMON_H_
#define _PSH_IA_COMMON_H_

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/circ_buf.h>
#include <linux/mutex.h>
#include <linux/completion.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/string.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>

#define CONFIG_SENSORS_PSH_BYT	1

#ifndef _CMD_ENGINE_H_
enum cmd_id {
	CMD_RESET,
	CMD_SETUP_DDR,
	CMD_GET_SINGLE,
	CMD_CFG_STREAM,
	CMD_STOP_STREAM,
	CMD_ADD_EVENT,
	CMD_CLEAR_EVENT,
	CMD_SELF_TEST,
	CMD_DEBUG,
	CMD_CALIBRATION,
	CMD_UPDATE_DDR,
	CMD_GET_STATUS,
	CMD_SET_PROPERTY,
	CMD_COUNTER,
	CMD_GET_VERSION,
	CMD_ID_MAX,
	CMD_FW_UPDATE = 255,
};

enum resp_type {
	RESP_CMD_ACK,
	RESP_GET_TIME,
	RESP_GET_SINGLE,
	RESP_STREAMING,
	RESP_DEBUG_MSG,
	RESP_DEBUG_GET_MASK = 5,
	RESP_GYRO_CAL_RESULT,
	RESP_BIST_RESULT,
	RESP_ADD_EVENT,
	RESP_CLEAR_EVENT,
	RESP_EVENT = 10,
	RESP_GET_STATUS,
	RESP_COMP_CAL_RESULT,
	RESP_COUNTER,
	RESP_GET_VERSION,
	RESP_TRACE_MSG,
};

#define CMD_PARAM_MAX_SIZE ((u16)60)
struct ia_cmd {
	u8 tran_id;
	u8 cmd_id;
	u8 sensor_id;
	char param[CMD_PARAM_MAX_SIZE];
} __packed;

struct cmd_resp {
	u8 tran_id;
	u8 type;
	u8 sensor_id;
	u16 data_len;
	char buf[0];
} __packed;

#define SCMD_DEBUG_SET_MASK ((u16)0x1)
#define SCMD_DEBUG_GET_MASK ((u16)0x2)
struct cmd_debug_param {
	u16 sub_cmd;
	u16 mask_out;
	u16 mask_level;
} __packed;

struct get_status_param {
	u32 snr_bitmask;
} __packed;

struct resp_debug_get_mask {
	u16 mask_out;
	u16 mask_level;
} __packed;

#define SCMD_GET_COUNTER ((u16)0x1)
#define SCMD_CLEAR_COUNTER ((u16)0x2)
struct cmd_counter_param {
	u16 sub_cmd;
} __packed;

struct resp_counter {
	u32 gpio_counter;
	u32 dma_counter;
	u32 i2c_counter;
	u32 print_counter;
} __packed;

#define VERSION_STR_MAX_SIZE ((u16)256)
struct resp_version {
	u8 str_len;
	char str[0];
} __packed;

#define LINK_AS_CLIENT		(0)
#define LINK_AS_MONITOR		(1)
#define LINK_AS_REPORTER	(2)
struct link_info {
	u8 id;
	u8 ltype;
	u16 slide;
} __packed;

#define SNR_NAME_MAX_LEN 6
struct snr_info {
	u8 id;
	u8 status;
	u16 freq;
	u16 data_cnt;
	u16 slide;
	u16 priv;
	u16 attri;

	u16 freq_max;
	char name[SNR_NAME_MAX_LEN];

	u8 health;
	u8 link_num;
	struct link_info linfo[0];
} __packed;
#define SNR_INFO_SIZE(sinfo) (sizeof(struct snr_info) \
		+ sinfo->link_num * sizeof(struct link_info))
#define SNR_INFO_MAX_SIZE 256

#define BUF_IA_DDR_SIZE 8192

#endif


#ifndef _SENSOR_DEF_H
struct sensor_cfg {
	u16 sample_freq; /* HZ */
	u16 buff_delay; /* max time(ms) for data bufferring */
	char extra[0];
} __packed;

#define SNR_RUNONLY_BITMASK ((u32)0x1 << 0)

#endif


#ifndef _LOOP_BUFFER_H_
struct psh_ia_priv;
typedef int (*update_finished_f)(struct psh_ia_priv *psh_ia_data,
					u16 offset);

struct loop_buffer {
	int in_reading;
	u8 *addr;
	u16 length;

	u16 off_head;
	u16 off_tail;

	update_finished_f update_finished;
};

#define LBUF_CELL_SIGN ((u16)0x4853)
#define LBUF_DISCARD_SIGN ((u16)0x4944)

struct frame_head {
	u16 sign;
	u16 length;
};

#define LBUF_MAX_CELL_SIZE ((u16)4096)
#define LBUF_MAX_DATA_SIZE (LBUF_MAX_CELL_SIZE \
	- 4 - 2 * sizeof(struct frame_head)\
	- sizeof(struct cmd_resp))

#define size_align(size) ((size % 4) ? (size + 4 - (size % 4)) : size)
#define frame_size(size) (size_align(size) + \
		sizeof(struct frame_head))
#endif

#define PSH2IA_CHANNEL0	0
#define PSH2IA_CHANNEL1	1
#define PSH2IA_CHANNEL2	2
#define PSH2IA_CHANNEL3	3

#define CIRC_SIZE (1024 * 64)

#define STR_BUFF_SIZE 256

struct psh_ia_priv {
	struct loop_buffer lbuf;	/* loop bufer */
	struct page *pg;
	struct circ_buf circ, circ_dbg;	/* circ buf for sysfs data node */
	struct resp_debug_get_mask dbg_mask;
	struct resp_counter counter;
	char *version_str;
	struct mutex cmd_mutex;
	struct completion cmpl;
	struct completion get_status_comp;
	struct completion cmd_reset_comp;
	struct completion cmd_load_comp;
	struct completion cmd_counter_comp;
	struct completion cmd_version_comp;
	struct list_head sensor_list;
	u32 reset_in_progress;
	u32 load_in_progress;
	u32 status_bitmask;

	void *platform_priv;
};

/* exports */
void ia_process_lbuf(struct device *dev);
int ia_send_cmd(struct psh_ia_priv *psh_ia_data,
		int ch, struct ia_cmd *cmd, int len);
int psh_ia_common_init(struct device *dev, struct psh_ia_priv **data);
void psh_ia_common_deinit(struct device *dev);


/* imports */
/* need implemented by user */
int do_setup_ddr(struct device *dev);
int process_send_cmd(struct psh_ia_priv *psh_ia_data,
			int ch, struct ia_cmd *cmd, int len);
int ia_handle_frame(struct psh_ia_priv *psh_ia_data, void *dbuf, int size);

#define PSH_ITSELF     (PHY_SENSOR_BASE) /* means PSH itself */
#define PORT_SENSOR_NUM (PORT_SENSOR_MAX_NUM - PORT_SENSOR_BASE - 1)
#define PORT_SENSOR_INDEX(x) ( \
		(x > PORT_SENSOR_BASE && x < PORT_SENSOR_MAX_NUM) \
		? (x - PORT_SENSOR_BASE - 1) : 0)


#define PSH_DBG_ALL     ((u16)-1)
#define PSH_DBG_FATAL   ((u16)(0x1 << 0x0))
#define PSH_DBG_ERR     ((u16)(0x1 << 0x1))
#define PSH_DBG_WARN    ((u16)(0x1 << 0x2))
#define PSH_DBG_INFO    ((u16)(0x1 << 0x3))
#define PSH_DBG_DBG     ((u16)(0x1 << 0x4))
#define PSH_DBG_CTRACE  ((u16)(0x1 << 0x5))     /* config path tracing */
#define PSH_DBG_DTRACE  ((u16)(0x1 << 0x6))     /* data path tracing */
#define PSH_DBG_MTRACE  ((u16)(0x1 << 0x7))     /* mutex_exec tracing */

/* port sensor is fixed, other sensor can be created dynamically */
enum sensor_type {
	PHY_SENSOR_BASE = 0,
	PORT_SENSOR_BASE = 200,
	CS_PORT,        /* port for streaming configuration and uploading */
	GS_PORT,        /* port for get_single configuration and uploading */
#if defined(CONFIG_HAVE_SENSOR_EVENT)
	EVT_PORT,       /* port for event configuration and uploading */
#endif
	PORT_SENSOR_MAX_NUM,
};

static const char sensor_port_str[PORT_SENSOR_NUM][SNR_NAME_MAX_LEN] = {
	"CSPRT",
	"GSPRT",
#if defined(CONFIG_HAVE_SENSOR_EVENT)
	"EVPRT",
#endif
};

struct sensor_db {
	u8 sid;
	char sensor_name[SNR_NAME_MAX_LEN];
	struct list_head list;
} __packed;

struct trace_data {
	u32 timestamp;
	u16 type;
	u16 event;
	u8 sensor_id;
	u8 sensor_cnt;
} __packed;

#endif
