/*
 * config.h
 *
 *  Created on: 13 Apr 2016
 *      Author: jakez
 */

#ifndef MY_CONFIG_H_
#define MY_CONFIG_H_

#define USED_UDP_PORT 45000
//#define USE_UDP
#define  MD5_HMAC

//comment those macros before running unit tests
//uncomment those macros after running unit tests
//otherwise these functions will never be invoked
#define _DEBUG //uncommnet this in release version
#define ENABLE_UNIT_TEST 1
#if ENABLE_UNIT_TEST == 1
#define MYSTATIC
#else
#define MYSTATIC static
#endif

#define enable_mock_dispatch_send_geco_packet 0
#define CURR_EVENT_LOG_LEVEL    DEBUG //INFO //DEBUG //VERBOSE
#endif /* MY_CONFIG_H_ */
