package org.yb.client;

import org.yb.annotations.InterfaceAudience;
import org.yb.master.MasterTypes;

@InterfaceAudience.Public
public class AlterUniverseReplicationResponse extends YRpcResponse {
  private final MasterTypes.MasterErrorPB serverError;

  public AlterUniverseReplicationResponse(
    long elapsedMillis, String tsUUID, MasterTypes.MasterErrorPB serverError) {
    super(elapsedMillis, tsUUID);
    this.serverError = serverError;
  }

  public boolean hasError() {
    return serverError != null;
  }

  public String errorMessage() {
    if (serverError == null) {
      return "";
    }

    return serverError.getStatus().getMessage();
  }
}
