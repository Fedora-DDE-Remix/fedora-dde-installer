#!/bin/bash

#include functions
for _FILE in $DI_INSTALL_TOOLS_DIR_ENV/functions/*.sh;do
    if [ -e "${_FILE}" ];then
         . "${_FILE}"
    fi
done

