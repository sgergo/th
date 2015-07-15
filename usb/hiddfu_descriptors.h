/*
 * hiddfu_descriptors.h
 *
 *  Created on: Jul 9, 2015
 *      Author: gsantha
 */

#ifndef HIDDFU_DESCRIPTORS_H_
#define HIDDFU_DESCRIPTORS_H_

#define DESCRIPTOR_BUFFER_SIZE  (COMPOSITE_DDFU_SIZE + COMPOSITE_DHID_SIZE)

extern tUSBDDFUDevice dfu_device;
extern tUSBDCompositeDevice composite_device;
extern uint8_t composite_descriptorbuffer[DESCRIPTOR_BUFFER_SIZE];
extern uint8_t const datapipereport_descriptor[];

#endif /* HIDDFU_DESCRIPTORS_H_ */
