/*
 * Copyright 2019 YugaByte, Inc. and Contributors
 *
 * Licensed under the Polyform Free Trial License 1.0.0 (the "License"); you
 * may not use this file except in compliance with the License. You
 * may obtain a copy of the License at
 *
 *     https://github.com/YugaByte/yugabyte-db/blob/master/licenses/POLYFORM-FREE-TRIAL-LICENSE-1.0.0.txt
 */

package com.yugabyte.yw.commissioner.tasks.subtasks;

import com.google.common.collect.ImmutableMap;
import com.google.common.net.HostAndPort;
import com.yugabyte.yw.commissioner.BaseTaskDependencies;
import com.yugabyte.yw.commissioner.tasks.UniverseTaskBase.ServerType;
import com.yugabyte.yw.commissioner.tasks.params.ServerSubTaskParams;
import java.util.Map;
import java.util.Map.Entry;
import javax.inject.Inject;
import lombok.extern.slf4j.Slf4j;
import org.yb.client.YBClient;

@Slf4j
public class SetFlagInMemory extends ServerSubTaskBase {

  // The gflag for tserver addresses.
  private static final String TSERVER_MASTER_ADDR_FLAG = "tserver_master_addrs";

  // The gflag for master addresses.
  private static final String MASTER_MASTER_ADDR_FLAG = "master_addresses";

  @Inject
  public SetFlagInMemory(BaseTaskDependencies baseTaskDependencies) {
    super(baseTaskDependencies);
  }

  // Parameters for wait task.
  public static class Params extends ServerSubTaskParams {
    // If the flags need to be forced to update (if it is not marked runtime safe).
    public boolean force = false;

    // The set of flags that need to be updated.
    public Map<String, String> gflags;

    // If ONLY the masters need to be updated.
    public boolean updateMasterAddrs = false;
  }

  @Override
  protected Params taskParams() {
    return (Params) taskParams;
  }

  @Override
  public void run() {
    checkParams();
    // Check if secondary IP is present, and if so, use that for the gflag value.
    String masterAddresses = getMasterAddresses(true);
    boolean isTserverTask = taskParams().serverType == ServerType.TSERVER;
    HostAndPort hp = getHostPort();

    Map<String, String> gflags = taskParams().gflags;
    if (taskParams().updateMasterAddrs) {
      String flagToSet = isTserverTask ? TSERVER_MASTER_ADDR_FLAG : MASTER_MASTER_ADDR_FLAG;
      gflags = ImmutableMap.of(flagToSet, masterAddresses);
    }
    YBClient client = getClient();
    try {
      if (gflags == null) {
        throw new IllegalArgumentException("Gflags cannot be null during a setFlag operation.");
      }
      for (Entry<String, String> gflag : gflags.entrySet()) {
        boolean setSuccess =
            client.setFlag(hp, gflag.getKey(), gflag.getValue(), taskParams().force);
        if (!setSuccess) {
          throw new RuntimeException(
              "Could not set gflag "
                  + gflag
                  + " for "
                  + taskParams().serverType
                  + " on node "
                  + taskParams().nodeName);
        }
      }
    } catch (Exception e) {
      log.error("{} hit error : {}", getName(), e.getMessage());
      throw new RuntimeException(e);
    } finally {
      closeClient(client);
    }
  }
}
