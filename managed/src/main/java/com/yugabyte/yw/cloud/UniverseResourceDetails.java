/*
 * Copyright 2019 YugaByte, Inc. and Contributors
 *
 * Licensed under the Polyform Free Trial License 1.0.0 (the "License"); you
 * may not use this file except in compliance with the License. You
 * may obtain a copy of the License at
 *
 * http://github.com/YugaByte/yugabyte-db/blob/master/licenses/POLYFORM-FREE-TRIAL-LICENSE-1.0.0.txt
 */

package com.yugabyte.yw.cloud;

import static com.yugabyte.yw.cloud.PublicCloudConstants.GP2_SIZE;
import static com.yugabyte.yw.cloud.PublicCloudConstants.GP3_PIOPS;
import static com.yugabyte.yw.cloud.PublicCloudConstants.GP3_SIZE;
import static com.yugabyte.yw.cloud.PublicCloudConstants.GP3_THROUGHPUT;
import static com.yugabyte.yw.cloud.PublicCloudConstants.IO1_PIOPS;
import static com.yugabyte.yw.cloud.PublicCloudConstants.IO1_SIZE;

import com.typesafe.config.Config;
import com.yugabyte.yw.commissioner.Common;
import com.yugabyte.yw.forms.UniverseDefinitionTaskParams;
import com.yugabyte.yw.forms.UniverseDefinitionTaskParams.Cluster;
import com.yugabyte.yw.forms.UniverseDefinitionTaskParams.UserIntent;
import com.yugabyte.yw.models.Customer;
import com.yugabyte.yw.models.InstanceType;
import com.yugabyte.yw.models.InstanceTypeKey;
import com.yugabyte.yw.models.PriceComponent;
import com.yugabyte.yw.models.PriceComponentKey;
import com.yugabyte.yw.models.Provider;
import com.yugabyte.yw.models.Region;
import com.yugabyte.yw.models.Universe;
import com.yugabyte.yw.models.helpers.NodeDetails;
import com.yugabyte.yw.models.helpers.ProviderAndRegion;
import io.swagger.annotations.ApiModelProperty;
import java.util.Collection;
import java.util.Collections;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.UUID;
import java.util.function.Function;
import java.util.stream.Collectors;
import lombok.Value;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class UniverseResourceDetails {
  public static final int MIB_IN_GIB = 1024;
  public static final String GP3_FREE_PIOPS_PARAM = "yb.aws.storage.gp3_free_piops";
  public static final String GP3_FREE_THROUGHPUT_PARAM = "yb.aws.storage.gp3_free_throughput";
  public static final Logger LOG = LoggerFactory.getLogger(UniverseResourceDetails.class);

  @ApiModelProperty(value = "Price per hour")
  public double pricePerHour = 0;

  @ApiModelProperty(value = "EBS price per hour")
  public double ebsPricePerHour = 0;

  @ApiModelProperty(value = "Numbers of cores")
  public double numCores = 0;

  @ApiModelProperty(value = "Memory GB")
  public double memSizeGB = 0;

  @ApiModelProperty(value = "Volume count")
  public int volumeCount = 0;

  @ApiModelProperty(value = "Volume in GB")
  public int volumeSizeGB = 0;

  @ApiModelProperty(value = "Numbers of node")
  public int numNodes = 0;

  @ApiModelProperty(value = "gp3 free piops")
  public int gp3FreePiops;

  @ApiModelProperty(value = "gp3 free throughput")
  public int gp3FreeThroughput;

  @ApiModelProperty(value = "Azs")
  public HashSet<String> azList = new HashSet<>();

  public void addCostPerHour(double price) {
    pricePerHour += price;
  }

  public void addEBSCostPerHour(double ebsPrice) {
    ebsPricePerHour += ebsPrice;
  }

  public void addVolumeCount(double volCount) {
    volumeCount += volCount;
  }

  public void addMemSizeGB(double memSize) {
    memSizeGB += memSize;
  }

  public void addVolumeSizeGB(double volSize) {
    volumeSizeGB += volSize;
  }

  public void addAz(String azName) {
    azList.add(azName);
  }

  public void addNumCores(double cores) {
    numCores += cores;
  }

  public void addNumNodes(int numNodes) {
    this.numNodes += numNodes;
  }

  public void addPrice(UniverseDefinitionTaskParams params, Context context) {

    // Calculate price
    double hourlyPrice = 0.0;
    double hourlyEBSPrice = 0.0;
    UserIntent userIntent = params.getPrimaryCluster().userIntent;
    for (NodeDetails nodeDetails : params.nodeDetailsSet) {
      if (nodeDetails.placementUuid != null) {
        userIntent = params.getClusterByUuid(nodeDetails.placementUuid).userIntent;
      }
      Provider provider = context.getProvider(UUID.fromString(userIntent.provider));
      if (!nodeDetails.isActive()) {
        continue;
      }
      Region region = context.getRegion(provider.uuid, nodeDetails.cloudInfo.region);

      if (region == null) {
        continue;
      }
      PriceComponent instancePrice =
          context.getPriceComponent(provider.uuid, region.code, userIntent.instanceType);
      if (instancePrice == null) {
        continue;
      }
      hourlyPrice += instancePrice.priceDetails.pricePerHour;

      // Add price of volumes if necessary
      // TODO: Remove aws check once GCP volumes are decoupled from "EBS" designation
      // TODO(wesley): gcp options?
      if (userIntent.deviceInfo.storageType != null
          && userIntent.providerType.equals(Common.CloudType.aws)) {
        Integer numVolumes = userIntent.deviceInfo.numVolumes;
        Integer diskIops = userIntent.deviceInfo.diskIops;
        Integer volumeSize = userIntent.deviceInfo.volumeSize;
        Integer throughput = userIntent.deviceInfo.throughput;
        Integer billedDiskIops = null;
        Integer billedThroughput = null;
        PriceComponent sizePrice = null;
        PriceComponent piopsPrice = null;
        PriceComponent mibpsPrice = null;
        switch (userIntent.deviceInfo.storageType) {
          case IO1:
            piopsPrice = PriceComponent.get(provider.uuid, region.code, IO1_PIOPS);
            sizePrice = PriceComponent.get(provider.uuid, region.code, IO1_SIZE);
            billedDiskIops = diskIops;
            break;
          case GP2:
            sizePrice = PriceComponent.get(provider.uuid, region.code, GP2_SIZE);
            break;
          case GP3:
            piopsPrice = PriceComponent.get(provider.uuid, region.code, GP3_PIOPS);
            sizePrice = PriceComponent.get(provider.uuid, region.code, GP3_SIZE);
            mibpsPrice = PriceComponent.get(provider.uuid, region.code, GP3_THROUGHPUT);
            billedDiskIops = diskIops > gp3FreePiops ? diskIops - gp3FreePiops : null;
            billedThroughput =
                throughput > gp3FreeThroughput ? throughput - gp3FreeThroughput : null;
            break;
          default:
            break;
        }
        if (sizePrice != null) {
          hourlyEBSPrice += (numVolumes * (volumeSize * sizePrice.priceDetails.pricePerHour));
        }
        if (piopsPrice != null && billedDiskIops != null) {
          hourlyEBSPrice += (numVolumes * (billedDiskIops * piopsPrice.priceDetails.pricePerHour));
        }
        if (mibpsPrice != null && billedThroughput != null) {
          hourlyEBSPrice +=
              (numVolumes * (billedThroughput * mibpsPrice.priceDetails.pricePerHour / MIB_IN_GIB));
        }
      }
    }
    hourlyPrice += hourlyEBSPrice;

    // Add price to details
    addCostPerHour(Double.parseDouble(String.format("%.4f", hourlyPrice)));
    addEBSCostPerHour(Double.parseDouble(String.format("%.4f", hourlyEBSPrice)));
  }

  public static UniverseResourceDetails create(
      UniverseDefinitionTaskParams params, Context context) {
    return create(params.nodeDetailsSet, params, context);
  }

  /**
   * Create a UniverseResourceDetails object, which contains info on the various pricing and other
   * sorts of resources used by this universe.
   *
   * @param nodes Nodes that make up this universe.
   * @param params Parameters describing this universe.
   * @return a UniverseResourceDetails object containing info on the universe's resources.
   */
  public static UniverseResourceDetails create(
      Collection<NodeDetails> nodes, UniverseDefinitionTaskParams params, Context context) {
    UniverseResourceDetails details = new UniverseResourceDetails();
    for (Cluster cluster : params.clusters) {
      details.addNumNodes(cluster.userIntent.numNodes);
    }
    UserIntent userIntent = params.getPrimaryCluster().userIntent;
    for (NodeDetails node : nodes) {
      if (node.isActive()) {
        if (node.placementUuid != null) {
          userIntent = params.getClusterByUuid(node.placementUuid).userIntent;
        }
        if (userIntent.deviceInfo != null
            && userIntent.deviceInfo.volumeSize != null
            && userIntent.deviceInfo.numVolumes != null) {
          details.addVolumeCount(userIntent.deviceInfo.numVolumes);
          details.addVolumeSizeGB(
              userIntent.deviceInfo.volumeSize * userIntent.deviceInfo.numVolumes);
        }
        if (node.cloudInfo != null
            && node.cloudInfo.az != null
            && node.cloudInfo.instance_type != null) {
          details.addAz(node.cloudInfo.az);
          InstanceType instanceType =
              context.getInstanceType(
                  UUID.fromString(userIntent.provider), node.cloudInfo.instance_type);
          if (instanceType == null) {
            LOG.error(
                "Couldn't find instance type "
                    + node.cloudInfo.instance_type
                    + " for provider "
                    + userIntent.providerType);
          } else {
            details.addMemSizeGB(instanceType.memSizeGB);
            details.addNumCores(instanceType.numCores);
          }
        }
      }
    }

    details.gp3FreePiops = context.getConfig().getInt(GP3_FREE_PIOPS_PARAM);
    details.gp3FreeThroughput = context.getConfig().getInt(GP3_FREE_THROUGHPUT_PARAM);
    details.addPrice(params, context);
    return details;
  }

  @Value
  public static class Context {
    Config config;
    Map<UUID, Provider> providerMap;
    Map<ProviderAndRegion, Region> regionsMap;
    Map<InstanceTypeKey, InstanceType> instanceTypeMap;
    Map<PriceComponentKey, PriceComponent> priceComponentMap;

    public Context(Config config, Universe universe) {
      this(
          config,
          Customer.get(universe.customerId),
          Collections.singletonList(universe.getUniverseDetails()));
    }

    public Context(Config config, Customer customer, UniverseDefinitionTaskParams universeParams) {
      this(config, customer, Collections.singletonList(universeParams));
    }

    public Context(
        Config config, Customer customer, Collection<UniverseDefinitionTaskParams> universeParams) {
      this.config = config;
      providerMap =
          Provider.getAll(customer.getUuid())
              .stream()
              .collect(Collectors.toMap(provider -> provider.uuid, Function.identity()));

      Set<InstanceTypeKey> instanceTypes =
          universeParams
              .stream()
              .filter(ud -> ud.nodeDetailsSet != null)
              .flatMap(
                  ud ->
                      ud.nodeDetailsSet
                          .stream()
                          .filter(NodeDetails::isActive)
                          .filter(nodeDetails -> nodeDetails.cloudInfo != null)
                          .filter(nodeDetails -> nodeDetails.cloudInfo.instance_type != null)
                          .map(
                              nodeDetails ->
                                  new InstanceTypeKey()
                                      .setProviderUuid(
                                          UUID.fromString(
                                              ud.getPrimaryCluster().userIntent.provider))
                                      .setInstanceTypeCode(nodeDetails.cloudInfo.instance_type)))
              .collect(Collectors.toSet());

      instanceTypeMap =
          InstanceType.findByKeys(instanceTypes)
              .stream()
              .collect(Collectors.toMap(InstanceType::getIdKey, Function.identity()));

      Set<ProviderAndRegion> providersAndRegions =
          universeParams
              .stream()
              .filter(ud -> ud.nodeDetailsSet != null)
              .flatMap(
                  ud ->
                      ud.nodeDetailsSet
                          .stream()
                          .filter(NodeDetails::isActive)
                          .filter(nodeDetails -> nodeDetails.cloudInfo != null)
                          .filter(nodeDetails -> nodeDetails.cloudInfo.region != null)
                          .map(
                              nodeDetails ->
                                  new ProviderAndRegion(
                                      UUID.fromString(ud.getPrimaryCluster().userIntent.provider),
                                      nodeDetails.getRegion())))
              .collect(Collectors.toSet());

      regionsMap =
          Region.findByKeys(providersAndRegions)
              .stream()
              .collect(Collectors.toMap(ProviderAndRegion::from, Function.identity()));

      priceComponentMap =
          PriceComponent.findByProvidersAndRegions(providersAndRegions)
              .stream()
              .collect(Collectors.toMap(PriceComponent::getIdKey, Function.identity()));
    }

    public Provider getProvider(UUID uuid) {
      return providerMap.get(uuid);
    }

    public Region getRegion(UUID providerUuid, String code) {
      return regionsMap.get(new ProviderAndRegion(providerUuid, code));
    }

    public InstanceType getInstanceType(UUID providerUuid, String code) {
      return instanceTypeMap.get(
          new InstanceTypeKey().setProviderUuid(providerUuid).setInstanceTypeCode(code));
    }

    public PriceComponent getPriceComponent(
        UUID providerUuid, String regionCode, String componentCode) {
      return priceComponentMap.get(
          PriceComponentKey.create(providerUuid, regionCode, componentCode));
    }
  }
}