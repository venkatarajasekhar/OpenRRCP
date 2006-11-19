/*
    This file is part of openrrcp

    OpenRRCP is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    OpenRRCP is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenRRCP; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    ---

    You can send your updates, patches and suggestions on this software
    to it's original author, Andrew Chernyak (nording@yandex.ru)
    This would be appreciated, however not required.
*/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "rrcp_io.h"
#include "rrcp_switches.h"
#include "rrcp_config.h"

const char *bandwidth_text[8]={"100M","128K","256K","512K","1M","2M","4M","8M"};
const char *wrr_ratio_text[4]={"4:1","8:1","16:1","1:0"};

struct t_swconfig swconfig;

////////////// read all relevant config from switch into our data structures    
void rrcp_config_read_from_switch(void)
{
    int i;
    
    swconfig.rrcp_config.raw=rtl83xx_readreg16(0x0200);
    for(i=0;i<2;i++)
	swconfig.rrcp_byport_disable.raw[i]=rtl83xx_readreg16(0x0201+i);
    for(i=0;i<14;i++){
	swconfig.vlan.raw[i]=rtl83xx_readreg16(0x030b+i);
    }
    for(i=0;i<4;i++){
	swconfig.vlan_port_output_tag.raw[i]=rtl83xx_readreg16(0x0319+i);
    }
    for(i=0;i<32;i++){
	swconfig.vlan_entry.raw[i*2]  =rtl83xx_readreg16(0x031d+i*3);
	swconfig.vlan_entry.raw[i*2+1]=rtl83xx_readreg16(0x031d+i*3+1);
	swconfig.vlan_vid[i]          =rtl83xx_readreg16(0x031d+i*3+2) & 0xfff;
	//mask out ports absent on this switch
	swconfig.vlan_entry.bitmap[i] &= 0xffffffff>>(32-switchtypes[switchtype].num_ports);
    }
    swconfig.vlan_port_insert_vid.raw[0]=rtl83xx_readreg16(0x037d);
    swconfig.vlan_port_insert_vid.raw[1]=rtl83xx_readreg16(0x037e);
    //mask out ports absent on this switch
    swconfig.vlan_port_insert_vid.bitmap &= 0xffffffff>>(32-switchtypes[switchtype].num_ports);
    for(i=0;i<16;i++){
	swconfig.bandwidth.raw[i]=rtl83xx_readreg16(0x020a+i);
    }
    for(i=0;i<6;i++){
	swconfig.port_monitor.raw[i]=rtl83xx_readreg16(0x0219+i);
    }
    for(i=0;i<2;i++){
	swconfig.alt.raw[i]=rtl83xx_readreg16(0x0300+i);
    }
    swconfig.qos_config.raw=rtl83xx_readreg16(0x0400);
    swconfig.qos_port_priority.raw[0]=rtl83xx_readreg16(0x0401);
    swconfig.qos_port_priority.raw[1]=rtl83xx_readreg16(0x0402);
    swconfig.port_config_global.raw=rtl83xx_readreg16(0x0607);
    for(i=0;i<2;i++){
	swconfig.port_disable.raw[i]=rtl83xx_readreg16(0x0608+i);
    }
    for(i=0;i<13;i++){
	swconfig.port_config.raw[i]=rtl83xx_readreg16(0x060a+i);
    }
}

void sncprintf(char *str, size_t size, const char *format, ...) {
    char line[1024];
    va_list ap;

    line[0]=0;
    va_start(ap, format);
    vsnprintf (line, size, format, ap);
    va_end(ap);
    if (size>(strlen(str)+strlen(line)+1)){
	strncat(str,line,sizeof(line));
    }
}

void rrcp_config_bin2text(char *s, int l)
{
    int i,port,port_phys,port2,port2_phys;

    sprintf(s,"!\n");
    {
        sncprintf(s,l,"version %s\n",switchtypes[switchtype].chip_name);
	sncprintf(s,l,"!\n");
    }
    {
	int mac_aging_time=-1;
	if (swconfig.alt.s.alt_config.mac_aging_disable){
	    mac_aging_time=0;
	}else if (swconfig.alt.s.alt_config.mac_aging_fast){
	    mac_aging_time=12;
	}else{
	    mac_aging_time=300;
	}
        sncprintf(s,l,"mac-address-table aging-time %d\n",mac_aging_time);
	sncprintf(s,l,"!\n");
    }
    {
	sncprintf(s,l,"%srrcp enable\n", swconfig.rrcp_config.config.rrcp_disable ? "no ":"");
	sncprintf(s,l,"%srrcp echo enable\n", swconfig.rrcp_config.config.echo_disable ? "no ":"");
	sncprintf(s,l,"%srrcp loop-detect enable\n", swconfig.rrcp_config.config.loop_enable ? "":"no ");
	sncprintf(s,l,"!\n");
    }
    {
	sncprintf(s,l,"%svlan enable\n", swconfig.vlan.s.config.enable ? "":"no ");
	sncprintf(s,l,"%svlan dot1q enable\n", swconfig.vlan.s.config.dot1q ? "":"no ");
	sncprintf(s,l,"%svlan leaky arp\n", swconfig.vlan.s.config.arp_leaky ? "":"no ");
	sncprintf(s,l,"%svlan leaky unicast\n", swconfig.vlan.s.config.unicast_leaky ? "":"no ");
	sncprintf(s,l,"%svlan leaky multicast\n", swconfig.vlan.s.config.multicast_leaky ? "":"no ");
	sncprintf(s,l,"%svlan untagged_frames drop\n", swconfig.vlan.s.config.drop_untagged_frames ? "":"no ");
	sncprintf(s,l,"%svlan invalid_vid drop\n", swconfig.vlan.s.config.ingress_filtering ? "":"no ");
	sncprintf(s,l,"!\n");
    }
    {
	sncprintf(s,l,"%sqos tos enable\n", swconfig.qos_config.config.tos_enable ? "":"no ");
	sncprintf(s,l,"%sqos dot1p enable\n", swconfig.qos_config.config.dot1p_enable ? "":"no ");
	sncprintf(s,l,"%sqos flow-control-jam enable\n", swconfig.qos_config.config.flow_control_jam ? "":"no ");
	sncprintf(s,l,"wrr-queue ratio %s\n", wrr_ratio_text[swconfig.qos_config.config.wrr_ratio]);
	sncprintf(s,l,"!\n");
    }
    {
	sncprintf(s,l,"%sflowcontrol dot3x enable\n", swconfig.port_config_global.config.flow_dot3x_disable ? "no ":"");
	sncprintf(s,l,"%sflowcontrol backpressure enable\n", swconfig.port_config_global.config.flow_backpressure_disable ? "no ":"");
	sncprintf(s,l,"%sstorm-control broadcast enable\n", swconfig.port_config_global.config.storm_control_broadcast_disable ? "no ":"");
	sncprintf(s,l,"%sstorm-control broadcast strict\n", swconfig.port_config_global.config.storm_control_broadcast_strict ? "":"no ");
	sncprintf(s,l,"%sstorm-control multicast strict\n", swconfig.port_config_global.config.storm_control_multicast_strict ? "":"no ");
	sncprintf(s,l,"!\n");
    }

    for(port=1;port<=switchtypes[switchtype].num_ports;port++){
	int is_trunk;
	port_phys=map_port_number_from_logical_to_physical(port);
	is_trunk=swconfig.vlan_port_insert_vid.bitmap&(1<<port_phys);
	sncprintf(s,l,"interface FastEthernet0/%d\n",port);
	sncprintf(s,l," %sshutdown\n", (swconfig.port_disable.bitmap&(1<<port_phys)) ? "":"no ");
	if (is_trunk){
	    char vlanlist[256],s[16];
	    vlanlist[0]=0;
	    for(i=0;i<32;i++){
		if(swconfig.vlan_entry.bitmap[i]&(1<<port_phys)){
		    sprintf(s,"%d",swconfig.vlan_vid[i]);
		    if (vlanlist[0]!=0){
			strcat(vlanlist,",");
		    }
		    strcat(vlanlist,s);
		}		
	    }
	    sncprintf(s,l," switchport trunk allowed vlan %s\n",vlanlist);
	}else{
	    sncprintf(s,l," switchport access vlan %d\n",swconfig.vlan_vid[swconfig.vlan.s.port_vlan_index[port_phys]]);
	}
	sncprintf(s,l," switchport mode %s\n",is_trunk ? "trunk":"access");
	sncprintf(s,l," rate-limit input %s\n",bandwidth_text[swconfig.bandwidth.rxtx[port_phys].rx]);
	sncprintf(s,l," rate-limit output %s\n",bandwidth_text[swconfig.bandwidth.rxtx[port_phys].tx]);
	if (swconfig.port_monitor.sniff.sniffer & (1<<port_phys)){
	    for(port2=1;port2<=switchtypes[switchtype].num_ports;port2++){
		port2_phys=map_port_number_from_logical_to_physical(port2);
		if ((swconfig.port_monitor.sniff.sniffed_rx & (1<<port2_phys))&&
		    (swconfig.port_monitor.sniff.sniffed_tx & (1<<port2_phys))){
		    sncprintf(s,l," port monitor FastEthernet0/%d\n", port2);
		}else if (swconfig.port_monitor.sniff.sniffed_rx & (1<<port2_phys)){
		    sncprintf(s,l," port monitor FastEthernet0/%d rx\n", port2);
		}else if (swconfig.port_monitor.sniff.sniffed_tx & (1<<port2_phys)){
		    sncprintf(s,l," port monitor FastEthernet0/%d tx\n", port2);
		}
	    }
	}
	if (swconfig.alt.s.alt_control&(1<<port_phys)){
	    sncprintf(s,l," no mac learning enable\n");
	}else{
	    sncprintf(s,l," mac learning enable\n");
	}
	if (swconfig.rrcp_byport_disable.bitmap&(1<<port_phys)){
	    sncprintf(s,l," no rrcp enable\n");
	}else{
	    sncprintf(s,l," rrcp enable\n");
	}
	sncprintf(s,l," mls qos cos %d\n", (swconfig.qos_port_priority.bitmap & (1<<port_phys)) ? 7:0);
	if (swconfig.port_config.config[port_phys].autoneg){
	    sncprintf(s,l," speed auto\n duplex auto\n");
	}else if (swconfig.port_config.config[port_phys].media_100full){
	    sncprintf(s,l," speed 100\n duplex full\n");
	}else if (swconfig.port_config.config[port_phys].media_100half){
	    sncprintf(s,l," speed 100\n duplex half\n");
	}else if (swconfig.port_config.config[port_phys].media_10full){
	    sncprintf(s,l," speed 10\n duplex full\n");
	}else if (swconfig.port_config.config[port_phys].media_10half){
	    sncprintf(s,l," speed 10\n duplex half\n");
	}
	sncprintf(s,l,"!\n");
    }
}

void do_show_config(void)
{
    char text[32768];

    rrcp_config_read_from_switch();
    rrcp_config_bin2text(text,sizeof(text));
    printf("%s",text);
}