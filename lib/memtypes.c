/*
 * Memory type definitions. This file is parsed by memtypes.awk to extract
 * MTYPE_ and memory_list_.. information in order to autogenerate 
 * memtypes.h.
 *
 * The script is sensitive to the format (though not whitespace), see
 * the top of memtypes.awk for more details.
 */

#include "kroute.h"
#include "memory.h"

struct memory_list memory_list_lib[] =
{
  { MTYPE_TMP,			"Temporary memory"		},
  { MTYPE_STRVEC,		"String vector"			},
  { MTYPE_VECTOR,		"Vector"			},
  { MTYPE_VECTOR_INDEX,		"Vector index"			},
  { MTYPE_LINK_LIST,		"Link List"			},
  { MTYPE_LINK_NODE,		"Link Node"			},
  { MTYPE_THREAD,		"Thread"			},
  { MTYPE_THREAD_MASTER,	"Thread master"			},
  { MTYPE_THREAD_STATS,		"Thread stats"			},
  { MTYPE_THREAD_FUNCNAME,	"Thread function name" 		},
  { MTYPE_VTY,			"VTY"				},
  { MTYPE_VTY_OUT_BUF,		"VTY output buffer"		},
  { MTYPE_VTY_HIST,		"VTY history"			},
  { MTYPE_IF,			"Interface"			},
  { MTYPE_CONNECTED,		"Connected" 			},
  { MTYPE_CONNECTED_LABEL,	"Connected interface label"	},
  { MTYPE_BUFFER,		"Buffer"			},
  { MTYPE_BUFFER_DATA,		"Buffer data"			},
  { MTYPE_STREAM,		"Stream"			},
  { MTYPE_STREAM_DATA,		"Stream data"			},
  { MTYPE_STREAM_FIFO,		"Stream FIFO"			},
  { MTYPE_PREFIX,		"Prefix"			},
  { MTYPE_PREFIX_IPV4,		"Prefix IPv4"			},
  { MTYPE_PREFIX_IPV6,		"Prefix IPv6"			},
  { MTYPE_HASH,			"Hash"				},
  { MTYPE_HASH_BACKET,		"Hash Bucket"			},
  { MTYPE_HASH_INDEX,		"Hash Index"			},
  { MTYPE_ROUTE_TABLE,		"Route table"			},
  { MTYPE_ROUTE_NODE,		"Route node"			},
  { MTYPE_DISTRIBUTE,		"Distribute list"		},
  { MTYPE_DISTRIBUTE_IFNAME,	"Dist-list ifname"		},
  { MTYPE_ACCESS_LIST,		"Access List"			},
  { MTYPE_ACCESS_LIST_STR,	"Access List Str"		},
  { MTYPE_ACCESS_FILTER,	"Access Filter"			},
  { MTYPE_PREFIX_LIST,		"Prefix List"			},
  { MTYPE_PREFIX_LIST_ENTRY,	"Prefix List Entry"		},
  { MTYPE_PREFIX_LIST_STR,	"Prefix List Str"		},
  { MTYPE_ROUTE_MAP,		"Route map"			},
  { MTYPE_ROUTE_MAP_NAME,	"Route map name"		},
  { MTYPE_ROUTE_MAP_INDEX,	"Route map index"		},
  { MTYPE_ROUTE_MAP_RULE,	"Route map rule"		},
  { MTYPE_ROUTE_MAP_RULE_STR,	"Route map rule str"		},
  { MTYPE_ROUTE_MAP_COMPILED,	"Route map compiled"		},
  { MTYPE_DESC,			"Command desc"			},
  { MTYPE_KEY,			"Key"				},
  { MTYPE_KEYCHAIN,		"Key chain"			},
  { MTYPE_IF_RMAP,		"Interface route map"		},
  { MTYPE_IF_RMAP_NAME,		"I.f. route map name",		},
  { MTYPE_SOCKUNION,		"Socket union"			},
  { MTYPE_PRIVS,		"Privilege information"		},
  { MTYPE_ZLOG,			"Logging"			},
  { MTYPE_ZCLIENT,		"Zclient"			},
  { MTYPE_WORK_QUEUE,		"Work queue"			},
  { MTYPE_WORK_QUEUE_ITEM,	"Work queue item"		},
  { MTYPE_WORK_QUEUE_NAME,	"Work queue name string"	},
  { MTYPE_PQUEUE,		"Priority queue"		},
  { MTYPE_PQUEUE_DATA,		"Priority queue data"		},
  { MTYPE_HOST,			"Host config"			},
  { -1, NULL },
};

struct memory_list memory_list_ha[] =
{
  { MTYPE_HA_TOP,			"HA Top"		},
  { MTYPE_HA_IF_INFO,       "HA Interface info"        },
  { -1, NULL },
};

struct mlist mlists[] __attribute__ ((unused)) = {
  { memory_list_lib,	"LIB"	},
  { memory_list_ha,    "HA"   },
  { NULL, NULL},
};
