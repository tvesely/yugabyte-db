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




/**
 * Contains the fault name and entity ref
 * @export
 * @interface FaultInjectionSpec
 */
export interface FaultInjectionSpec  {
  /**
   * 
   * @type {string}
   * @memberof FaultInjectionSpec
   */
  fault_name: string;
  /**
   * 
   * @type {string}
   * @memberof FaultInjectionSpec
   */
  entity_ref: string;
  /**
   * 
   * @type {string}
   * @memberof FaultInjectionSpec
   */
  user_id?: string;
}


