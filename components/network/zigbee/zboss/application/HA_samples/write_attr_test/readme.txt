    Test steps:

/* **************** Verifying initial attribute values **************** */
0) Send command 'Read Attributes' for each 'Time' cluster attributes
   and receive responses that contain the following parameters:

   *----------------------------*-----------------------*-------------*------------*
   |  Attribute                 |  Status               |  Data Type  |  Value     |
   *----------------------------*-----------------------*-------------*------------*
   | Time                       | Success               | UTC         | 0x00000000 |
   | Time Zone                  | Success               | int32       | 0x00000000 |
   | Daylight Saving Time Start | Success               | uint32      | 0x00000000 |
   | Daylight Saving Time End   | Success               | uint32      | 0x00000000 |
   | Standard Time              | Success               | uint32      | 0x00000000 |
   | Valid Until Time           | Unsupported Attribute |  -          |  -         |
   *----------------------------*-----------------------*-------------*------------*

/* **************** 'Write Attributes' - 4 VALID records **************** */
1) Send standard 'Write Attributes' command for each 'Time' cluster attribute
   and receive a response with the 'Success' status

   *----------------------------*-------------*------------*
   |  Attribute                 |  Data Type  |  Value     |
   *----------------------------*-------------*------------*
   | Time                       | UTC         | 0x00000001 |
   | Time Zone                  | int32       | 0x00000001 |
   | Daylight Saving Time Start | uint32      | 0x00000001 |
   | Daylight Saving Time End   | uint32      | 0x00000001 |
   *----------------------------*-------------*------------*

     Verifying attribute values
   Send 'Read Attributes' command for each 'Time' cluster attribute and receive
   responses that contain the following parameters:

   *----------------------------*----------*-------------*------------*
   |  Attribute                 |  Status  |  Data Type  |  Value     |
   *----------------------------*----------*-------------*------------*
   | Time                       | Success  | UTC         | 0x00000001 |
   | Time Zone                  | Success  | int32       | 0x00000001 |
   | Daylight Saving Time Start | Success  | uint32      | 0x00000001 |
   | Daylight Saving Time End   | Success  | uint32      | 0x00000001 |
   *----------------------------*----------*-------------*------------*

/* **************** 'Write Attributes' - 4 INVALID records **************** */
2) Send standard 'Write Attributes' command for each 'Time' cluster attribute
   that contain the following parameters and receive a response with following
   statuses:

   *--------------------------*-------------*------------*-----------------------*
   |  Attribute               |  Data Type  |  Value     |  Response status      |
   *--------------------------*-------------*------------*-----------------------*
   | Valid Until Time         | UTC         | 0x00000002 | Unsupported Attribute |
   | Time Zone                | uint32      | 0x00000002 | Invalid Data Type     |
   | Standard Time            | uint32      | 0x00000002 | Read Only             |
   | Daylight Saving Time End | uint32      | 0xffffffff | Invalid Value         |
   *--------------------------*-------------*------------*-----------------------*

     Verifying attribute values
   Send 'Read Attributes' command for each 'Time' cluster attribute and receive
   responses that contain the following parameters:

   *----------------------------*-----------------------*-------------*------------*
   |  Attribute                 |  Status               |  Data Type  |  Value     |
   *----------------------------*-----------------------*-------------*------------*
   | Valid Until Time           | Unsupported Attribute |  -          |  -         |
   | Time Zone                  | Success               | int32       | 0x00000001 |
   | Standard  Time             | Success               | uint32      | 0x00000001 |
   | Daylight Saving Time End   | Success               | uint32      | 0x00000001 |
   *----------------------------*-----------------------*-------------*------------*

/* **************** 'Write Attributes' - 2 VALID and 2 INVALID records **************** */
3) Send standard 'Write Attributes' command for each 'Time' cluster attribute
   that contain the following parameters and receive a response with following
   statuses:

   *--------------------------*-------------*------------*-------------------*
   |  Attribute               |  Data Type  |  Value     |  Response status  |
   *--------------------------*-------------*------------*-------------------*
   | Time                     | UTC         | 0x00000003 |  -                |
   | Time Zone                | int32       | 0x00000003 |  -                |
   | Standard Time            | uint32      | 0x00000003 | Read Only         |
   | Daylight Saving Time End | uint32      | 0xffffffff | Invalid Value     |
   *--------------------------*-------------*------------*-------------------*

     Verifying attribute values
   Send 'Read Attributes' command for each 'Time' cluster attribute and receive
   responses that contain the following parameters:

   *--------------------------*----------*-------------*------------*
   |  Attribute               |  Status  |  Data Type  |  Value     |
   *--------------------------*----------*-------------*------------*
   | Time                     | Success  | UTC         | 0x00000003 |
   | Time Zone                | Success  | int32       | 0x00000003 |
   | Standard  Time           | Success  | uint32      | 0x00000004 |
   | Daylight Saving Time End | Success  | uint32      | 0x00000001 |
   *--------------------------*----------*-------------*------------*

/* **************** 'Write Attributes Undivided' - 4 VALID records **************** */
4) Send standard 'Write Attributes Undivided' command for each 'Time' cluster attribute
   and receive a response with the 'Success' status

   *----------------------------*-------------*------------*
   |  Attribute                 |  Data Type  |  Value     |
   *----------------------------*-------------*------------*
   | Time                       | UTC         | 0x00000004 |
   | Time Zone                  | int32       | 0x00000004 |
   | Daylight Saving Time Start | uint32      | 0x00000004 |
   | Daylight Saving Time End   | uint32      | 0x00000004 |
   *----------------------------*-------------*------------*

     Verifying attribute values
   Send 'Read Attributes' command for each 'Time' cluster attribute and receive
   responses that contain the following parameters:

   *----------------------------*----------*-------------*------------*
   |  Attribute                 |  Status  |  Data Type  |  Value     |
   *----------------------------*----------*-------------*------------*
   | Time                       | Success  | UTC         | 0x00000004 |
   | Time Zone                  | Success  | int32       | 0x00000004 |
   | Daylight Saving Time Start | Success  | uint32      | 0x00000004 |
   | Daylight Saving Time End   | Success  | uint32      | 0x00000004 |
   *----------------------------*----------*-------------*------------*

/* **************** 'Write Attributes Undivided' - 4 INVALID records **************** */
5) Send standard 'Write Attributes Undivided' command for each 'Time' cluster attribute
   that contain the following parameters and receive a response with following
   statuses:

   *--------------------------*-------------*------------*-----------------------*
   |  Attribute               |  Data Type  |  Value     |  Response status      |
   *--------------------------*-------------*------------*-----------------------*
   | Valid Until Time         | UTC         | 0x00000005 | Unsupported Attribute |
   | Time Zone                | uint32      | 0x00000005 | Invalid Data Type     |
   | Standard Time            | uint32      | 0x00000005 | Read Only             |
   | Daylight Saving Time End | uint32      | 0xffffffff | Invalid Value         |
   *--------------------------*-------------*------------*-----------------------*

     Verifying attribute values
   Send 'Read Attributes' command for each 'Time' cluster attribute and receive
   responses that contain the following parameters:

   *----------------------------*-----------------------*-------------*------------*
   |  Attribute                 |  Status               |  Data Type  |  Value     |
   *----------------------------*-----------------------*-------------*------------*
   | Valid Until Time           | Unsupported Attribute |  -          |  -         |
   | Time Zone                  | Success               | int32       | 0x00000004 |
   | Standard  Time             | Success               | uint32      | 0x00000007 |
   | Daylight Saving Time End   | Success               | uint32      | 0x00000004 |
   *----------------------------*-----------------------*-------------*------------*

/* **************** 'Write Attributes Undivided' - 2 VALID and 2 INVALID records **************** */
6) Send standard 'Write Attributes Undivided' command for each 'Time' cluster attribute
   that contain the following parameters and receive a response with following
   statuses:

   *--------------------------*-------------*------------*-------------------*
   |  Attribute               |  Data Type  |  Value     |  Response status  |
   *--------------------------*-------------*------------*-------------------*
   | Time                     | UTC         | 0x00000006 |  -                |
   | Time Zone                | int32       | 0x00000006 |  -                |
   | Standard Time            | uint32      | 0x00000006 | Read Only         |
   | Daylight Saving Time End | uint32      | 0xffffffff | Invalid Value     |
   *--------------------------*-------------*------------*-------------------*

     Verifying attribute values
   Send 'Read Attributes' command for each 'Time' cluster attribute and receive
   responses that contain the following parameters:

   *--------------------------*----------*-------------*------------*
   |  Attribute               |  Status  |  Data Type  |  Value     |
   *--------------------------*----------*-------------*------------*
   | Time                     | Success  | UTC         | 0x00000004 |
   | Time Zone                | Success  | int32       | 0x00000004 |
   | Standard  Time           | Success  | uint32      | 0x0000000a |
   | Daylight Saving Time End | Success  | uint32      | 0x00000004 |
   *--------------------------*----------*-------------*------------*

/* **************** 'Write Attributes No Response' - 4 VALID records **************** */
7) Send standard 'Write Attributes No Response' command for each 'Time' cluster
   attribute and DON'T receive a response

   *----------------------------*-------------*------------*
   |  Attribute                 |  Data Type  |  Value     |
   *----------------------------*-------------*------------*
   | Time                       | UTC         | 0x00000007 |
   | Time Zone                  | int32       | 0x00000007 |
   | Daylight Saving Time Start | uint32      | 0x00000007 |
   | Daylight Saving Time End   | uint32      | 0x00000007 |
   *----------------------------*-------------*------------*

     Verifying attribute values
   Send 'Read Attributes' command for each 'Time' cluster attribute and receive
   responses that contain the following parameters:

   *----------------------------*----------*-------------*------------*
   |  Attribute                 |  Status  |  Data Type  |  Value     |
   *----------------------------*----------*-------------*------------*
   | Time                       | Success  | UTC         | 0x00000007 |
   | Time Zone                  | Success  | int32       | 0x00000007 |
   | Daylight Saving Time Start | Success  | uint32      | 0x00000007 |
   | Daylight Saving Time End   | Success  | uint32      | 0x00000007 |
   *----------------------------*----------*-------------*------------*

/* **************** 'Write Attributes No Response' - 4 INVALID records **************** */
8) Send standard 'Write Attributes No Response' command for each 'Time' cluster attribute
   that contain the following parameters and DON'T receive a response

   *--------------------------*-------------*------------*
   |  Attribute               |  Data Type  |  Value     |
   *--------------------------*-------------*------------*
   | Valid Until Time         | UTC         | 0x00000008 |
   | Time Zone                | uint32      | 0x00000008 |
   | Standard Time            | uint32      | 0x00000008 |
   | Daylight Saving Time End | uint32      | 0xffffffff |
   *--------------------------*-------------*------------*

     Verifying attribute values
   Send 'Read Attributes' command for each 'Time' cluster attribute and receive
   responses that contain the following parameters:

   *----------------------------*-----------------------*-------------*------------*
   |  Attribute                 |  Status               |  Data Type  |  Value     |
   *----------------------------*-----------------------*-------------*------------*
   | Valid Until Time           | Unsupported Attribute |  -          |  -         |
   | Time Zone                  | Success               | int32       | 0x00000007 |
   | Standard  Time             | Success               | uint32      | 0x0000000b |
   | Daylight Saving Time End   | Success               | uint32      | 0x00000007 |
   *----------------------------*-----------------------*-------------*------------*

/* **************** 'Write Attributes No Response' - 2 VALID and 2 INVALID records **************** */
9) Send standard 'Write Attributes No Response' command for each 'Time' cluster attribute
   that contain the following parameters and DON'T receive a response

   *--------------------------*-------------*------------*-------------------*
   |  Attribute               |  Data Type  |  Value     |  Response status  |
   *--------------------------*-------------*------------*-------------------*
   | Time                     | UTC         | 0x00000009 |  -                |
   | Time Zone                | int32       | 0x00000009 |  -                |
   | Standard Time            | uint32      | 0x00000009 | Read Only         |
   | Daylight Saving Time End | uint32      | 0xffffffff | Invalid Value     |
   *--------------------------*-------------*------------*-------------------*

     Verifying attribute values
   Send 'Read Attributes' command for each 'Time' cluster attribute and receive
   responses that contain the following parameters:

   *--------------------------*----------*-------------*------------*
   |  Attribute               |  Status  |  Data Type  |  Value     |
   *--------------------------*----------*-------------*------------*
   | Time                     | Success  | UTC         | 0x00000009 |
   | Time Zone                | Success  | int32       | 0x00000009 |
   | Standard  Time           | Success  | uint32      | 0x00000010 |
   | Daylight Saving Time End | Success  | uint32      | 0x00000007 |
   *--------------------------*----------*-------------*------------*
