#!/bin/sh
. $1/.config

ARCH_DIR=$1/arch/mips

if [ "$CONFIG_LOG_BUF_LEN" == "" ]; then
	CONFIG_LOG_BUF_LEN=0
fi

if [ "$CONFIG_LOG_SINGLE_POOL" == "" ]; then
	POOL_NUM=2
else
	POOL_NUM=1
fi

if [ "$CONFIG_NET_CLUSTER_GROUP2" == "" ]; then
	CONFIG_NET_CLUSTER_4K_USAGE=0
fi

sed -e "s/RAM_SIZE/${CONFIG_XROUTER_RAM_SIZE}/g" $ARCH_DIR/target.ld.tmp \
	| sed -e "s/MBUF_SIZE/${CONFIG_NET_MBUF_USAGE}/g" \
	| sed -e "s/MCL_SIZE/${CONFIG_NET_CLUSTER_USAGE}/g" \
	| sed -e "s/MCL_4K_SIZE/${CONFIG_NET_CLUSTER_4K_USAGE}/g" \
	| sed -e "s/LOG_SIZE/${CONFIG_LOG_BUF_LEN}/g" \
	| sed -e "s/LOG_POOLS/${POOL_NUM}/g" \
	> $1/target.ld

cp -f $ARCH_DIR/zload.ld zload/zload.ld
cp -f $ARCH_DIR/start.S init/start.S
cp -f $ARCH_DIR/load.S zload/load.S




