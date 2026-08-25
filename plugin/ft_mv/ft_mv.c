/* Copyright (c) 2026, MariaDB plc

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1335 USA */

/*
  Full-text parser plugin for multi valued indexes.
*/

#include <my_global.h>
#include <mysql/plugin.h>
#include <json_lib.h>

#define MAX_TOKEN 512   /* maximum bytes in one emitted token */

/* -------------------------------------------------------------------------
   Emit one token to the FTS engine.
   ---------------------------------------------------------------------- */

static void emit_word(MYSQL_FTPARSER_PARAM *param, const char *word, int len)
{
  MYSQL_FTPARSER_BOOLEAN_INFO bool_info= { FT_TOKEN_WORD, 0, 0, 0, 0, ' ', 0 };
  param->mysql_add_word(param, word, len, &bool_info);
}


/* -------------------------------------------------------------------------
   For each scalar value found by json_get_path_next, construct a
   "path=value" token and emit it.
   ---------------------------------------------------------------------- */

static void emit_token(MYSQL_FTPARSER_PARAM *param,
                       json_engine_t *je, json_path_t *p)
{
  char  word[MAX_TOKEN + 1];
  int   val_len;
  uchar val_buf[MAX_TOKEN];

  if (je->value_type == JSON_VALUE_STRING)
  {
    val_len= json_unescape(je->s.cs, je->value, je->value + je->value_len,
                           je->s.cs, val_buf, val_buf + MAX_TOKEN);
    if (val_len < 0)
      val_len= 0;
  }
  else
  {
    /* NUMBER, TRUE, FALSE, NULL — raw bytes are plain ASCII */
    val_len= je->value_len;
    if (val_len > MAX_TOKEN)
      val_len= MAX_TOKEN;
    memcpy(val_buf, je->value, (size_t) val_len);
  }

  if (val_len > 0)
  {
    memcpy(word, val_buf, (size_t) val_len);
  }

  emit_word(param, word, val_len);
}

/* -------------------------------------------------------------------------
   Main parse entry point.
   ---------------------------------------------------------------------- */

static int ft_mv_parse(MYSQL_FTPARSER_PARAM *param)
{
  int              je_stack[JSON_DEPTH_DEFAULT];
  json_path_step_t p_steps[JSON_DEPTH_DEFAULT];
  json_engine_t    je;
  json_path_t      p;

  if (param->mode == MYSQL_FTPARSER_FULL_BOOLEAN_INFO)
  {
    param->mysql_parse(param, param->doc, param->length);
    return 0;
  }

  /*
    Initialise json_engine_t and json_path_t using stack-allocated buffers.
    MY_BUFFER_NO_RESIZE keeps all accesses in-bounds without any heap
    allocation.  json_lib's JSON_DEPTH_LIMIT guard ensures stack_p and
    last_step_idx stay within [0, JSON_DEPTH_DEFAULT-1].
  */
  memset(&je, 0, sizeof(je));
  memset(&p,  0, sizeof(p));

  je.stack.buffer=           (uchar *) je_stack;
  je.stack.max_element=      JSON_DEPTH_DEFAULT;
  je.stack.size_of_element=  sizeof(int);
  je.stack.malloc_flags=     MY_BUFFER_NO_RESIZE;

  p.steps.buffer=            (uchar *) p_steps;
  p.steps.max_element=       JSON_DEPTH_DEFAULT;
  p.steps.size_of_element=   sizeof(json_path_step_t);
  p.steps.malloc_flags=      MY_BUFFER_NO_RESIZE;

  /*
    Tell the FTS engine to copy each word we emit: our token buffer is a
    local stack variable that is overwritten on every iteration, so the
    engine must not hold a bare pointer into it.
  */
  param->flags|= MYSQL_FTFLAGS_NEED_COPY;

  json_get_path_start(&je, param->cs, (const uchar *) param->doc,
                      (const uchar *) param->doc + param->length, &p);

  while (json_get_path_next(&je, &p) == 0)
  {
    if (!json_value_scalar(&je))
      continue;
    emit_token(param, &je, &p);
  }

  return 0;
}

/* -------------------------------------------------------------------------
   Plugin boilerplate.
   ---------------------------------------------------------------------- */

static struct st_mysql_ftparser ft_mv_descriptor=
{
  MYSQL_FTPARSER_INTERFACE_VERSION,
  ft_mv_parse,
  NULL,
  NULL
};

maria_declare_plugin(ft_mv)
{
  MYSQL_FTPARSER_PLUGIN,
  &ft_mv_descriptor,
  "ft_mv",
  "MariaDB plc",
  "Full-text parser for multi valued indexes",
  PLUGIN_LICENSE_GPL,
  NULL,
  NULL,
  0x0100,
  NULL,
  NULL,
  "1.0",
  MariaDB_PLUGIN_MATURITY_ALPHA
}
maria_declare_plugin_end;
