#include <kroute.h>

#include "memory.h"
#include "thread.h"
#include "prefix.h"
#include "table.h"
#include "vty.h"
#include "command.h"
#include "plist.h"
#include "log.h"
#include "zclient.h"

#include "ha_deamon.h"

static struct cmd_node ha_node =
{
  HA_NODE,
  "%s(config-ha)# ",
  1
};

DEFUN (ha,
       ha_cmd,
       "ha",
       "Start HA configuration\n")
{
  vty->node = HA_NODE;
//  vty->index = ha_get ();
 
  return CMD_SUCCESS;
}

DEFUN (no_ha,
       no_ha_cmd,
       "no ha",
       NO_STR
       "Start HA configuration\n")
{
  struct ha *ha;

  ha = ha_lookup ();
  if (ha == NULL)
    {
      vty_out (vty, "There isn't active ha instance%s", VTY_NEWLINE);
      return CMD_WARNING;
    }

  ha_finish (ha);

  return CMD_SUCCESS;
}

/* HA configuration write function. */
static int
ha_config_write (struct vty *vty)
{
    int write = 0;

    return write;
}

void
ha_vty_show_init (void)
{

}

/* Install HA related vty commands. */
void
ha_vty_init (void)
{
  /* Install ha top node. */
  install_node (&ha_node, ha_config_write);

  /* ha commands. */
  install_element (CONFIG_NODE, &ha_cmd);
  install_element (CONFIG_NODE, &no_ha_cmd);
}

