/*
 * test-mpath.cc
 *
 *  Created on: 17Feb.,2017
 *      Author: jackiez
 */

#include <iostream>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "geco-test.h"

struct mpath : public testing::Test
{
	path_controller_t* mpath_;
	void* old_arg3;
	timeout_t  old_exps;
	uint old_retrans_count;
	bool old_hb_sent;
	geco_channel_t* old_channel;
	timeout* timerID;
	path_params_t* path;
	bool old_hb_acked;
	uint old_total_retrans_count;

	virtual void SetUp()
	{
		alloc_geco_channel();
		mpath_ = curr_channel_->path_control;
		path = &mpath_->path_params[0];
		timerID = path->hb_timer_id;
		old_arg3 = timerID->callback.arg3;
		old_exps = timerID->expires;
		old_retrans_count = path->retrans_count;
		old_hb_sent = path->hb_sent;
		old_hb_acked = path->hb_acked;
		old_channel = curr_channel_;
		old_total_retrans_count = mpath_->total_retrans_count;
	}
	virtual void TearDown()
	{
		free_geco_channel();
	}

	void reset_path()
	{
		//reset everything of 'path' to its init valuess
		mpath_->total_retrans_count = old_total_retrans_count;
		path->hb_sent = old_hb_sent;
		timerID->callback.arg3 = old_arg3;
		path->retrans_count = old_retrans_count;
		timerID->expires = old_exps;
		path->hb_sent = old_hb_sent;
		path->hb_acked = old_hb_acked;
		curr_channel_ = old_channel;
		curr_geco_instance_ = curr_channel_->geco_inst;
	}
};

TEST_F(mpath, test_new_and_free)
{
	ASSERT_EQ(mpath_->channel_id, curr_channel_->channel_id);
	ASSERT_EQ(mpath_->primary_path, UT_PRI_PATH_ID);
	ASSERT_EQ(mpath_->path_num, UT_REMOTE_ADDR_LIST_SIZE);
	ASSERT_EQ(mpath_->max_retrans_per_path, 2);
	ASSERT_EQ(mpath_->rto_initial, curr_geco_instance_->default_rtoInitial);
	ASSERT_EQ(mpath_->rto_min, curr_geco_instance_->default_rtoMin);
	ASSERT_EQ(mpath_->rto_max, curr_geco_instance_->default_rtoMax);
	ASSERT_EQ(mpath_->min_pmtu, PMTU_LOWEST);
	ASSERT_NE(mpath_->path_params, nullptr);
}

TEST_F(mpath, test_set_paths)
{
	ASSERT_EQ(mpath_->primary_path, UT_PRI_PATH_ID);
	ASSERT_EQ(mpath_->path_num, UT_REMOTE_ADDR_LIST_SIZE);
	ASSERT_EQ(mpath_->total_retrans_count, 0);
	ASSERT_EQ(mpath_->max_retrans_per_path, 2);
	ASSERT_EQ(curr_channel_->state_machine_control->max_assoc_retrans_count, mpath_->max_retrans_per_path*UT_REMOTE_ADDR_LIST_SIZE);
	for (int i = 0; i < mpath_->path_num; i++)
	{
		ASSERT_EQ(mpath_->path_params[i].hb_enabled, true);
		ASSERT_EQ(mpath_->path_params[i].firstRTO, true);
		ASSERT_EQ(mpath_->path_params[i].retrans_count, 0);
		ASSERT_EQ(mpath_->path_params[i].rto, mpath_->rto_initial);
		ASSERT_EQ(mpath_->path_params[i].srtt, mpath_->rto_initial);
		ASSERT_EQ(mpath_->path_params[i].rttvar, 0);
		ASSERT_EQ(mpath_->path_params[i].hb_sent, false);
		ASSERT_EQ(mpath_->path_params[i].hb_acked, false);
		ASSERT_EQ(mpath_->path_params[i].timer_backoff, false);
		ASSERT_EQ(mpath_->path_params[i].dchunk_acked_in_last_rto, false);
		ASSERT_EQ(mpath_->path_params[i].dchunk_sent_in_last_rto, false);
		ASSERT_EQ(mpath_->path_params[i].hb_interval, PM_INITIAL_HB_INTERVAL);
		ASSERT_EQ(mpath_->path_params[i].path_id, i);
		ASSERT_EQ(mpath_->path_params[i].eff_pmtu, PMTU_LOWEST);
		ASSERT_EQ(mpath_->path_params[i].probing_pmtu, PMTU_HIGHEST);
		if (i != mpath_->primary_path)
			ASSERT_EQ(mpath_->path_params[i].state, PM_PATH_UNCONFIRMED);
		else
			ASSERT_EQ(mpath_->path_params[i].state, PM_ACTIVE);
		ASSERT_NE(mpath_->path_params[i].hb_timer_id, nullptr);
	}
}

bool
mpath_handle_chunks_rtx(short pathID);
TEST_F(mpath, test_handle_chunks_retx)
{
	//given max_channel_retrans_count = 2,max_retrans_per_path = 1
	bool ret;
	//1 when path is unconfirmed,
	mpath_->path_params[0].state = PM_PATH_UNCONFIRMED;
	mpath_handle_chunks_rtx(0);
	//then only increment path retrans counter by one
	ASSERT_EQ(mpath_->path_params[0].retrans_count, 1);
	//reset to initial mpath value
	mpath_->path_params[0].retrans_count = 0;
	mpath_->path_params[0].state = PM_PATH_UNCONFIRMED;
	//2 when path is active,
	mpath_->path_params[1].state = PM_ACTIVE;
	mpath_handle_chunks_rtx(1);
	//then increment both path and channel retrans counters by one
	ASSERT_EQ(mpath_->path_params[1].retrans_count, 1);
	ASSERT_EQ(mpath_->total_retrans_count, 1);
	//reset to initial mpath value
	mpath_->path_params[1].retrans_count = 0;
	mpath_->path_params[1].state = PM_PATH_UNCONFIRMED;
	mpath_->total_retrans_count = 0;
	//3 when total_retrans_count >= max_channel_retrans_count,
	mpath_->path_params[0].state = PM_ACTIVE;
	mpath_->path_params[1].state = PM_ACTIVE;
	ret = mpath_handle_chunks_rtx(0);
	ret = mpath_handle_chunks_rtx(1);
	ret = mpath_handle_chunks_rtx(0);
	ret = mpath_handle_chunks_rtx(1);
	// then disconnect and delete channel -> return true
	ASSERT_EQ(curr_channel_, nullptr);
	ASSERT_EQ(curr_geco_instance_, nullptr);
	ASSERT_EQ(ret, true);
	// as the last test exceeds max_channel_retrans_count that leads to curr_channel_ freed so realloc it
	alloc_geco_channel();
	mpath_ = curr_channel_->path_control;
	//4 when path is inactive,
	mpath_->path_params[0].state = PM_INACTIVE;
	ret = mpath_handle_chunks_rtx(0);
	// then stop -> return false
	ASSERT_EQ(ret, false);
	//reset to initial mpath value
	mpath_->path_params[0].state = PM_PATH_UNCONFIRMED;
	//5 when path max retrans  >= max_retrans_per_path
	mpath_->path_params[0].state = PM_ACTIVE;
	mpath_handle_chunks_rtx(0);
	mpath_handle_chunks_rtx(0);
	// then path is marked as inactive
	ASSERT_EQ(mpath_->path_params[0].state, PM_INACTIVE);
	//reset to initial mpath value
	mpath_->path_params[0].retrans_count = 0;
	mpath_->path_params[0].state = PM_PATH_UNCONFIRMED;
	mpath_->total_retrans_count = 0;
	//6 when there is active or unconfirmed path
	mpath_->path_params[1].state = PM_ACTIVE;
	mpath_handle_chunks_rtx(1);
	mpath_handle_chunks_rtx(1);
	mpath_handle_chunks_rtx(0);
	// then channel is still active
	ASSERT_EQ(mpath_->path_params[1].state, PM_INACTIVE);
	ASSERT_EQ(mpath_->path_params[0].state, PM_PATH_UNCONFIRMED);
	ASSERT_EQ(mpath_->total_retrans_count, 2);
	//reset to initial mpath value
	mpath_->path_params[0].retrans_count = 0;
	mpath_->path_params[0].state = PM_PATH_UNCONFIRMED;
	mpath_->path_params[1].retrans_count = 0;
	mpath_->path_params[1].state = PM_PATH_UNCONFIRMED;
	mpath_->total_retrans_count = 0;
	//7 when unconfirmed paths' total_retrans_count >= max_channel_retrans_count,
	ret = mpath_handle_chunks_rtx(0);
	ret = mpath_handle_chunks_rtx(1);
	ret = mpath_handle_chunks_rtx(0);
	ret = mpath_handle_chunks_rtx(1);
	// then disconnect and delete channel -> return true
	ASSERT_EQ(curr_channel_, nullptr);
	ASSERT_EQ(curr_geco_instance_, nullptr);
	ASSERT_EQ(ret, true);
	// as the last test exceeds max_channel_retrans_count that leads to curr_channel_ freed so realloc it
	alloc_geco_channel();
	mpath_ = curr_channel_->path_control;
	//8 when one active one unconfirmed paths' total_retrans_count >= max_channel_retrans_count,
	mpath_->path_params[0].state = PM_ACTIVE;
	ret = mpath_handle_chunks_rtx(0);
	ret = mpath_handle_chunks_rtx(1);
	ret = mpath_handle_chunks_rtx(0);
	ret = mpath_handle_chunks_rtx(1);
	// then disconnect and delete channel -> return true
	ASSERT_EQ(curr_channel_, nullptr);
	ASSERT_EQ(curr_geco_instance_, nullptr);
	ASSERT_EQ(ret, true);
	// as the last test exceeds max_channel_retrans_count that leads to curr_channel_ freed so realloc it
	alloc_geco_channel();
	mpath_ = curr_channel_->path_control;
	//9 when primary path becomes inactive,
	mpath_->primary_path = 0;
	mpath_->path_params[0].dchunk_sent_in_last_rto = true;
	ret = mpath_handle_chunks_rtx(0);
	ret = mpath_handle_chunks_rtx(0);
	// then use path1 as primary path even it is unconfirmed
	ASSERT_EQ(mpath_->primary_path, 1);
	ASSERT_EQ(mpath_->path_params[0].dchunk_sent_in_last_rto, false);
	ASSERT_EQ(mpath_->path_params[1].dchunk_acked_in_last_rto, false);
	//reset to initial mpath value
	mpath_->path_params[0].dchunk_sent_in_last_rto = false;
	mpath_->path_params[0].retrans_count = 0;
	mpath_->path_params[0].state = PM_PATH_UNCONFIRMED;
	mpath_->path_params[1].retrans_count = 0;
	mpath_->path_params[1].state = PM_PATH_UNCONFIRMED;
	mpath_->total_retrans_count = 0;
}

extern int mpath_heartbeat_timer_expired(timeout* timerID);
extern timeouts* mtra_read_timeouts();
TEST_F(mpath, test_heartbeat_timer_expired)
{
	bool ret;
	//1 when mtu 0, hb_sent false, hb_acked false
	timerID->callback.arg3 = nullptr;
	path->hb_sent = false;
	mpath_heartbeat_timer_expired(timerID);
	//then this is the error case that should not happen 
	// in order to make it rubust, send hb probe with mtu 0 again
	ASSERT_EQ(curr_channel_, nullptr);
	ASSERT_EQ(timerID->callback.arg3, nullptr);
	ASSERT_EQ(timerID, path->hb_timer_id);
	ASSERT_EQ(false, path->dchunk_sent_in_last_rto);
	ASSERT_EQ(false, path->dchunk_acked_in_last_rto);
	ASSERT_EQ(path->retrans_count, 0);
	ASSERT_LE(abs((timerID->expires - old_exps) / stamps_per_ms_double() - (double)(path->rto + path->hb_interval)), 0.1f);
	reset_path();
	//2 when mtu 0, hb_sent true, hb_acked false
	timerID->callback.arg3 = nullptr;
	path->hb_sent = true;
	mpath_heartbeat_timer_expired(timerID);
	//then this is case that last hb probe failed and need send hb probe with mtu 0 again
	ASSERT_EQ(curr_channel_, nullptr);
	ASSERT_EQ(timerID->callback.arg3, nullptr);
	ASSERT_EQ(path->probing_pmtu, PMTU_HIGHEST);
	ASSERT_EQ(timerID, path->hb_timer_id);
	ASSERT_EQ(false, path->dchunk_sent_in_last_rto);
	ASSERT_EQ(false, path->dchunk_acked_in_last_rto);
	ASSERT_EQ(path->retrans_count, 1);
	ASSERT_LE(abs((timerID->expires - old_exps) / stamps_per_ms_double() - (double)(path->rto + path->hb_interval)), 0.1f);
	reset_path();
	//3 when mtu !0, hb_sent false, hb_acked false
	path->hb_sent = false;
	mpath_heartbeat_timer_expired(timerID);
	//then path verify when connected by sending hb probe with PMTU_HIGHEST
	ASSERT_EQ(curr_channel_, nullptr);
	ASSERT_EQ(*(uint*)(timerID->callback.arg3), PMTU_HIGHEST);
	ASSERT_EQ(path->probing_pmtu, PMTU_HIGHEST);
	ASSERT_EQ(timerID, path->hb_timer_id);
	ASSERT_EQ(false, path->dchunk_sent_in_last_rto);
	ASSERT_EQ(false, path->dchunk_acked_in_last_rto);
	ASSERT_EQ(path->retrans_count, 0);
	ASSERT_LE(abs((timerID->expires - old_exps) / stamps_per_ms_double() - (double)(path->rto)), 0.1f);
	reset_path();
	//4 when mtu !0, hb_sent false, hb_acked true
	path->hb_sent = false;
	path->hb_acked = true;
	mpath_heartbeat_timer_expired(timerID);
	//then pmtu&hb probe suceeds, switch to normal hb probe by sending hb probe again
	ASSERT_EQ(curr_channel_, nullptr);
	ASSERT_EQ(timerID->callback.arg3, nullptr);
	ASSERT_EQ(path->probing_pmtu, PMTU_HIGHEST);
	ASSERT_EQ(timerID, path->hb_timer_id);
	ASSERT_EQ(false, path->dchunk_sent_in_last_rto);
	ASSERT_EQ(false, path->dchunk_acked_in_last_rto);
	ASSERT_EQ(path->retrans_count, 0);
	ASSERT_LE(abs((timerID->expires - old_exps) / stamps_per_ms_double() - (double)(path->rto + path->hb_interval)), 0.1f);
	reset_path();
	//5 when total_retrans_count >= max_channel_retrans_count,
	mpath_->total_retrans_count = mpath_->max_retrans_per_path*mpath_->path_num;
	mpath_->path_params[1].state = PM_ACTIVE;
	path->hb_sent = true;
	path->hb_acked = false;
	mpath_heartbeat_timer_expired(timerID);
	// then disconnect and delete channel -> return true
	ASSERT_EQ(curr_channel_, nullptr);
	ASSERT_EQ(curr_geco_instance_, nullptr);
	// realloc channel and reset evenrything
	this->SetUp();
}

extern void mpath_update_rtt(short pathID, int newRTT);
TEST_F(mpath, test_update_rtt)
{
	bool ret;
	//1 when it is retransmit acked
	mpath_update_rtt(path->path_id, 0);
	//then should not update rtt  pmData->path_params[pathID].firstRTO
	ASSERT_EQ(path->firstRTO, true);
	reset_path();
	//2 when it is Not retransmit acked
	mpath_update_rtt(path->path_id, 50);
	//then should update rtt
	ASSERT_EQ(path->firstRTO, false);
	ASSERT_EQ(path->srtt, 50);
	ASSERT_EQ(path->rttvar, 25);
	ASSERT_EQ(path->rto, mpath_->rto_min);
	mpath_update_rtt(path->path_id, 50);
	//then should update rtt
	ASSERT_EQ(path->firstRTO, false);
	ASSERT_EQ(path->srtt, 50);
	ASSERT_EQ(path->rttvar, 18);
	ASSERT_EQ(path->rto, mpath_->rto_min);
	reset_path();
}
