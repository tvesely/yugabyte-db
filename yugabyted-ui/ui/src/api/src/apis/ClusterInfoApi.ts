// tslint:disable
/**
 * Yugabyte Cloud
 * YugabyteDB as a Service
 *
 * The version of the OpenAPI document: v1
 * Contact: support@yugabyte.com
 *
 * NOTE: This class is auto generated by OpenAPI Generator (https://openapi-generator.tech).
 * https://openapi-generator.tech
 * Do not edit the class manually.
 */

// eslint-disable-next-line @typescript-eslint/ban-ts-comment
// @ts-ignore
import { useQuery, useInfiniteQuery, useMutation, UseQueryOptions, UseInfiniteQueryOptions, UseMutationOptions } from 'react-query';
import Axios from '../runtime';
import type { AxiosInstance } from 'axios';
// eslint-disable-next-line @typescript-eslint/ban-ts-comment
// @ts-ignore
import type {
  ApiError,
  ClusterInstantMetricsListResponse,
  ClusterNodesResponse,
  ClusterTableListResponse,
  ClusterTablespacesListResponse,
  LiveQueryResponseSchema,
  MetricResponse,
  SlowQueryResponseSchema,
} from '../models';

export interface GetBulkClusterMetricsForQuery {
  accountId: string;
  projectId: string;
  cluster_ids: Set<string>;
}
export interface GetClusterMetricForQuery {
  metrics: string;
  node_name?: string;
  region?: string;
  start_time?: number;
  end_time?: number;
}
export interface GetClusterTablesForQuery {
  accountId: string;
  projectId: string;
  clusterId: string;
  api?: GetClusterTablesApiEnum;
}
export interface GetClusterTablespacesForQuery {
  accountId: string;
  projectId: string;
  clusterId: string;
}
export interface GetLiveQueriesForQuery {
  api?: GetLiveQueriesApiEnum;
}

/**
 * Get bulk cluster metrics
 * Get bulk cluster metrics
 */

export const getBulkClusterMetricsAxiosRequest = (
  requestParameters: GetBulkClusterMetricsForQuery,
  customAxiosInstance?: AxiosInstance
) => {
  return Axios<ClusterInstantMetricsListResponse>(
    {
      url: '/public/accounts/{accountId}/projects/{projectId}/cluster_metrics'.replace(`{${'accountId'}}`, encodeURIComponent(String(requestParameters.accountId))).replace(`{${'projectId'}}`, encodeURIComponent(String(requestParameters.projectId))),
      method: 'GET',
      params: {
        cluster_ids: requestParameters['cluster_ids'],
      }
    },
    customAxiosInstance
  );
};

export const getBulkClusterMetricsQueryKey = (
  requestParametersQuery: GetBulkClusterMetricsForQuery,
  pageParam = -1,
  version = 1,
) => [
  `/v${version}/public/accounts/{accountId}/projects/{projectId}/cluster_metrics`,
  pageParam,
  ...(requestParametersQuery ? [requestParametersQuery] : [])
];


export const useGetBulkClusterMetricsInfiniteQuery = <T = ClusterInstantMetricsListResponse, Error = ApiError>(
  params: GetBulkClusterMetricsForQuery,
  options?: {
    query?: UseInfiniteQueryOptions<ClusterInstantMetricsListResponse, Error, T>;
    customAxiosInstance?: AxiosInstance;
  },
  pageParam = -1,
  version = 1,
) => {
  const queryKey = getBulkClusterMetricsQueryKey(params, pageParam, version);
  const { query: queryOptions, customAxiosInstance } = options ?? {};

  const query = useInfiniteQuery<ClusterInstantMetricsListResponse, Error, T>(
    queryKey,
    () => getBulkClusterMetricsAxiosRequest(params, customAxiosInstance),
    queryOptions
  );

  return {
    queryKey,
    ...query
  };
};

export const useGetBulkClusterMetricsQuery = <T = ClusterInstantMetricsListResponse, Error = ApiError>(
  params: GetBulkClusterMetricsForQuery,
  options?: {
    query?: UseQueryOptions<ClusterInstantMetricsListResponse, Error, T>;
    customAxiosInstance?: AxiosInstance;
  },
  version = 1,
) => {
  const queryKey = getBulkClusterMetricsQueryKey(params,  version);
  const { query: queryOptions, customAxiosInstance } = options ?? {};

  const query = useQuery<ClusterInstantMetricsListResponse, Error, T>(
    queryKey,
    () => getBulkClusterMetricsAxiosRequest(params, customAxiosInstance),
    queryOptions
  );

  return {
    queryKey,
    ...query
  };
};



/**
 * Get metrics for a Yugabyte cluster
 * Get a metric for a cluster
 */

export const getClusterMetricAxiosRequest = (
  requestParameters: GetClusterMetricForQuery,
  customAxiosInstance?: AxiosInstance
) => {
  return Axios<MetricResponse>(
    {
      url: '/metrics',
      method: 'GET',
      params: {
        metrics: requestParameters['metrics'],
        node_name: requestParameters['node_name'],
        region: requestParameters['region'],
        start_time: requestParameters['start_time'],
        end_time: requestParameters['end_time'],
      }
    },
    customAxiosInstance
  );
};

export const getClusterMetricQueryKey = (
  requestParametersQuery: GetClusterMetricForQuery,
  pageParam = -1,
  version = 1,
) => [
  `/v${version}/metrics`,
  pageParam,
  ...(requestParametersQuery ? [requestParametersQuery] : [])
];


export const useGetClusterMetricInfiniteQuery = <T = MetricResponse, Error = ApiError>(
  params: GetClusterMetricForQuery,
  options?: {
    query?: UseInfiniteQueryOptions<MetricResponse, Error, T>;
    customAxiosInstance?: AxiosInstance;
  },
  pageParam = -1,
  version = 1,
) => {
  const queryKey = getClusterMetricQueryKey(params, pageParam, version);
  const { query: queryOptions, customAxiosInstance } = options ?? {};

  const query = useInfiniteQuery<MetricResponse, Error, T>(
    queryKey,
    () => getClusterMetricAxiosRequest(params, customAxiosInstance),
    queryOptions
  );

  return {
    queryKey,
    ...query
  };
};

export const useGetClusterMetricQuery = <T = MetricResponse, Error = ApiError>(
  params: GetClusterMetricForQuery,
  options?: {
    query?: UseQueryOptions<MetricResponse, Error, T>;
    customAxiosInstance?: AxiosInstance;
  },
  version = 1,
) => {
  const queryKey = getClusterMetricQueryKey(params,  version);
  const { query: queryOptions, customAxiosInstance } = options ?? {};

  const query = useQuery<MetricResponse, Error, T>(
    queryKey,
    () => getClusterMetricAxiosRequest(params, customAxiosInstance),
    queryOptions
  );

  return {
    queryKey,
    ...query
  };
};



/**
 * Get nodes for a Yugabyte cluster
 * Get the nodes for a cluster
 */

export const getClusterNodesAxiosRequest = (
  customAxiosInstance?: AxiosInstance
) => {
  return Axios<ClusterNodesResponse>(
    {
      url: '/nodes',
      method: 'GET',
      params: {
      }
    },
    customAxiosInstance
  );
};

export const getClusterNodesQueryKey = (
  pageParam = -1,
  version = 1,
) => [
  `/v${version}/nodes`,
  pageParam,
];


export const useGetClusterNodesInfiniteQuery = <T = ClusterNodesResponse, Error = ApiError>(
  options?: {
    query?: UseInfiniteQueryOptions<ClusterNodesResponse, Error, T>;
    customAxiosInstance?: AxiosInstance;
  },
  pageParam = -1,
  version = 1,
) => {
  const queryKey = getClusterNodesQueryKey(pageParam, version);
  const { query: queryOptions, customAxiosInstance } = options ?? {};

  const query = useInfiniteQuery<ClusterNodesResponse, Error, T>(
    queryKey,
    () => getClusterNodesAxiosRequest(customAxiosInstance),
    queryOptions
  );

  return {
    queryKey,
    ...query
  };
};

export const useGetClusterNodesQuery = <T = ClusterNodesResponse, Error = ApiError>(
  options?: {
    query?: UseQueryOptions<ClusterNodesResponse, Error, T>;
    customAxiosInstance?: AxiosInstance;
  },
  version = 1,
) => {
  const queryKey = getClusterNodesQueryKey(version);
  const { query: queryOptions, customAxiosInstance } = options ?? {};

  const query = useQuery<ClusterNodesResponse, Error, T>(
    queryKey,
    () => getClusterNodesAxiosRequest(customAxiosInstance),
    queryOptions
  );

  return {
    queryKey,
    ...query
  };
};



/**
 * Get list of tables per YB API (YCQL/YSQL)
 * Get list of DB tables per YB API (YCQL/YSQL)
 */

export const getClusterTablesAxiosRequest = (
  requestParameters: GetClusterTablesForQuery,
  customAxiosInstance?: AxiosInstance
) => {
  return Axios<ClusterTableListResponse>(
    {
      url: '/public/accounts/{accountId}/projects/{projectId}/clusters/{clusterId}/tables'.replace(`{${'accountId'}}`, encodeURIComponent(String(requestParameters.accountId))).replace(`{${'projectId'}}`, encodeURIComponent(String(requestParameters.projectId))).replace(`{${'clusterId'}}`, encodeURIComponent(String(requestParameters.clusterId))),
      method: 'GET',
      params: {
        api: requestParameters['api'],
      }
    },
    customAxiosInstance
  );
};

export const getClusterTablesQueryKey = (
  requestParametersQuery: GetClusterTablesForQuery,
  pageParam = -1,
  version = 1,
) => [
  `/v${version}/public/accounts/{accountId}/projects/{projectId}/clusters/{clusterId}/tables`,
  pageParam,
  ...(requestParametersQuery ? [requestParametersQuery] : [])
];


export const useGetClusterTablesInfiniteQuery = <T = ClusterTableListResponse, Error = ApiError>(
  params: GetClusterTablesForQuery,
  options?: {
    query?: UseInfiniteQueryOptions<ClusterTableListResponse, Error, T>;
    customAxiosInstance?: AxiosInstance;
  },
  pageParam = -1,
  version = 1,
) => {
  const queryKey = getClusterTablesQueryKey(params, pageParam, version);
  const { query: queryOptions, customAxiosInstance } = options ?? {};

  const query = useInfiniteQuery<ClusterTableListResponse, Error, T>(
    queryKey,
    () => getClusterTablesAxiosRequest(params, customAxiosInstance),
    queryOptions
  );

  return {
    queryKey,
    ...query
  };
};

export const useGetClusterTablesQuery = <T = ClusterTableListResponse, Error = ApiError>(
  params: GetClusterTablesForQuery,
  options?: {
    query?: UseQueryOptions<ClusterTableListResponse, Error, T>;
    customAxiosInstance?: AxiosInstance;
  },
  version = 1,
) => {
  const queryKey = getClusterTablesQueryKey(params,  version);
  const { query: queryOptions, customAxiosInstance } = options ?? {};

  const query = useQuery<ClusterTableListResponse, Error, T>(
    queryKey,
    () => getClusterTablesAxiosRequest(params, customAxiosInstance),
    queryOptions
  );

  return {
    queryKey,
    ...query
  };
};



/**
 * Get list of tablespaces for YSQL
 * Get list of DB tables for YSQL
 */

export const getClusterTablespacesAxiosRequest = (
  requestParameters: GetClusterTablespacesForQuery,
  customAxiosInstance?: AxiosInstance
) => {
  return Axios<ClusterTablespacesListResponse>(
    {
      url: '/public/accounts/{accountId}/projects/{projectId}/clusters/{clusterId}/tablespaces'.replace(`{${'accountId'}}`, encodeURIComponent(String(requestParameters.accountId))).replace(`{${'projectId'}}`, encodeURIComponent(String(requestParameters.projectId))).replace(`{${'clusterId'}}`, encodeURIComponent(String(requestParameters.clusterId))),
      method: 'GET',
      params: {
      }
    },
    customAxiosInstance
  );
};

export const getClusterTablespacesQueryKey = (
  requestParametersQuery: GetClusterTablespacesForQuery,
  pageParam = -1,
  version = 1,
) => [
  `/v${version}/public/accounts/{accountId}/projects/{projectId}/clusters/{clusterId}/tablespaces`,
  pageParam,
  ...(requestParametersQuery ? [requestParametersQuery] : [])
];


export const useGetClusterTablespacesInfiniteQuery = <T = ClusterTablespacesListResponse, Error = ApiError>(
  params: GetClusterTablespacesForQuery,
  options?: {
    query?: UseInfiniteQueryOptions<ClusterTablespacesListResponse, Error, T>;
    customAxiosInstance?: AxiosInstance;
  },
  pageParam = -1,
  version = 1,
) => {
  const queryKey = getClusterTablespacesQueryKey(params, pageParam, version);
  const { query: queryOptions, customAxiosInstance } = options ?? {};

  const query = useInfiniteQuery<ClusterTablespacesListResponse, Error, T>(
    queryKey,
    () => getClusterTablespacesAxiosRequest(params, customAxiosInstance),
    queryOptions
  );

  return {
    queryKey,
    ...query
  };
};

export const useGetClusterTablespacesQuery = <T = ClusterTablespacesListResponse, Error = ApiError>(
  params: GetClusterTablespacesForQuery,
  options?: {
    query?: UseQueryOptions<ClusterTablespacesListResponse, Error, T>;
    customAxiosInstance?: AxiosInstance;
  },
  version = 1,
) => {
  const queryKey = getClusterTablespacesQueryKey(params,  version);
  const { query: queryOptions, customAxiosInstance } = options ?? {};

  const query = useQuery<ClusterTablespacesListResponse, Error, T>(
    queryKey,
    () => getClusterTablespacesAxiosRequest(params, customAxiosInstance),
    queryOptions
  );

  return {
    queryKey,
    ...query
  };
};



/**
 * Get the Live Queries in a Yugabyte Cluster
 * Get the live queries in a cluster
 */

export const getLiveQueriesAxiosRequest = (
  requestParameters: GetLiveQueriesForQuery,
  customAxiosInstance?: AxiosInstance
) => {
  return Axios<LiveQueryResponseSchema>(
    {
      url: '/live_queries',
      method: 'GET',
      params: {
        api: requestParameters['api'],
      }
    },
    customAxiosInstance
  );
};

export const getLiveQueriesQueryKey = (
  requestParametersQuery: GetLiveQueriesForQuery,
  pageParam = -1,
  version = 1,
) => [
  `/v${version}/live_queries`,
  pageParam,
  ...(requestParametersQuery ? [requestParametersQuery] : [])
];


export const useGetLiveQueriesInfiniteQuery = <T = LiveQueryResponseSchema, Error = ApiError>(
  params: GetLiveQueriesForQuery,
  options?: {
    query?: UseInfiniteQueryOptions<LiveQueryResponseSchema, Error, T>;
    customAxiosInstance?: AxiosInstance;
  },
  pageParam = -1,
  version = 1,
) => {
  const queryKey = getLiveQueriesQueryKey(params, pageParam, version);
  const { query: queryOptions, customAxiosInstance } = options ?? {};

  const query = useInfiniteQuery<LiveQueryResponseSchema, Error, T>(
    queryKey,
    () => getLiveQueriesAxiosRequest(params, customAxiosInstance),
    queryOptions
  );

  return {
    queryKey,
    ...query
  };
};

export const useGetLiveQueriesQuery = <T = LiveQueryResponseSchema, Error = ApiError>(
  params: GetLiveQueriesForQuery,
  options?: {
    query?: UseQueryOptions<LiveQueryResponseSchema, Error, T>;
    customAxiosInstance?: AxiosInstance;
  },
  version = 1,
) => {
  const queryKey = getLiveQueriesQueryKey(params,  version);
  const { query: queryOptions, customAxiosInstance } = options ?? {};

  const query = useQuery<LiveQueryResponseSchema, Error, T>(
    queryKey,
    () => getLiveQueriesAxiosRequest(params, customAxiosInstance),
    queryOptions
  );

  return {
    queryKey,
    ...query
  };
};



/**
 * Get the Slow Queries in a Yugabyte Cluster
 * Get the slow queries in a cluster
 */

export const getSlowQueriesAxiosRequest = (
  customAxiosInstance?: AxiosInstance
) => {
  return Axios<SlowQueryResponseSchema>(
    {
      url: '/slow_queries',
      method: 'GET',
      params: {
      }
    },
    customAxiosInstance
  );
};

export const getSlowQueriesQueryKey = (
  pageParam = -1,
  version = 1,
) => [
  `/v${version}/slow_queries`,
  pageParam,
];


export const useGetSlowQueriesInfiniteQuery = <T = SlowQueryResponseSchema, Error = ApiError>(
  options?: {
    query?: UseInfiniteQueryOptions<SlowQueryResponseSchema, Error, T>;
    customAxiosInstance?: AxiosInstance;
  },
  pageParam = -1,
  version = 1,
) => {
  const queryKey = getSlowQueriesQueryKey(pageParam, version);
  const { query: queryOptions, customAxiosInstance } = options ?? {};

  const query = useInfiniteQuery<SlowQueryResponseSchema, Error, T>(
    queryKey,
    () => getSlowQueriesAxiosRequest(customAxiosInstance),
    queryOptions
  );

  return {
    queryKey,
    ...query
  };
};

export const useGetSlowQueriesQuery = <T = SlowQueryResponseSchema, Error = ApiError>(
  options?: {
    query?: UseQueryOptions<SlowQueryResponseSchema, Error, T>;
    customAxiosInstance?: AxiosInstance;
  },
  version = 1,
) => {
  const queryKey = getSlowQueriesQueryKey(version);
  const { query: queryOptions, customAxiosInstance } = options ?? {};

  const query = useQuery<SlowQueryResponseSchema, Error, T>(
    queryKey,
    () => getSlowQueriesAxiosRequest(customAxiosInstance),
    queryOptions
  );

  return {
    queryKey,
    ...query
  };
};







/**
  * @export
  * @enum {string}
  */
export enum GetClusterTablesApiEnum {
  Ycql = 'YCQL',
  Ysql = 'YSQL'
}
/**
  * @export
  * @enum {string}
  */
export enum GetLiveQueriesApiEnum {
  Ysql = 'YSQL',
  Ycql = 'YCQL'
}