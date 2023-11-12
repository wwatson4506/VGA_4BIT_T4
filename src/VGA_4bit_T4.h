/*
   This file is part of VGA_4bit_T4 library.

   VGA_4bit_T4 library is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   VGA_4bit_T4 library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with VGA_4bit_T4 library. If not, see <http://www.gnu.org/licenses/>.

   Copyright (C) 2023 Warren Watson

   Original Author: Watson <wwatson4506@gmail.com>

  This is a modified version of a 4 bit VGA driver using flexio by jmarsh.
  https://forum.pjrc.com/threads/72710-VGA-output-via-FlexIO-(Teensy4).
  Various graphic and text functions taken from uVGA by Eric PREVOTEAU
  and adapted for use with VGA_4bit_T4.
  https://github.com/qix67/uVGA.
*/

#ifndef _VGA_4BIT_T4_H
#define _VGA_4BIT_T4_H

#include "Arduino.h"
#include <DMAChannel.h>
#include "VGA_T4_Config.h"
#include "box.h"

/* R2R ladder:
 *
 * GROUND <------------- 536R ----*---- 270R -----*---------> VGA PIN: Red=1
 *                                |               |
 * INTENSITY (13) <---536R -------/               |
 *                                                |
 * T4-11 <---536R --------------------------------/
 *
 * GROUND <------------- 536R ----*---- 270R -----*---------> VGA PIN: Green=2
 *                                |               |
 * INTENSITY (13) <---536R -------/               |
 *                                                |
 * T4-12 <---536R --------------------------------/
 *
 * GROUND <------------- 536R ----*---- 270R -----*---------> VGA PIN: Blue=3
 *                                |               |
 * INTENSITY (13) <---536R -------/               |
 *                                                |
 * T4-10 <---536R --------------------------------/
 *
 * VSYNC (T34) <---------------68R---------------------------> VGA PIN 14
 *
 * HSYNC (T35) <---------------68R---------------------------> VGA PIN 13
 */

// Color defines for 4 bit VGA.
#define VGA_BLACK  0
#define VGA_BLUE    1
#define VGA_GREEN   2
#define VGA_CYAN    3
#define VGA_RED     4
#define VGA_MAGENTA 5
#define VGA_YELLOW  6
#define VGA_WHITE   7
#define VGA_GREY    8
#define VGA_BRIGHT_BLUE    9
#define VGA_BRIGHT_GREEN   10
#define VGA_BRIGHT_CYAN    11
#define VGA_BRIGHT_RED     12
#define VGA_BRIGHT_MAGENTA 13 // MAGENTA ?
#define VGA_BRIGHT_YELLOW  14
#define VGA_BRIGHT_WHITE   15

#define GET_L_NIBBLE(l) l >> 4 & 0x0f
#define GET_R_NIBBLE(r) r & 0x0f

typedef enum vga_text_direction
{
  VGA_DIR_RIGHT,
  VGA_DIR_TOP,
  VGA_DIR_LEFT,
  VGA_DIR_BOTTOM,
} vga_text_direction;

#define MEMSRC 0   // Font source from memory.
#define FILESRC 1  // Font source from file.

//**************************************************************//
// Graphic Cursor Image Array
//**************************************************************//
const uint8_t arrow[][128] = {
 {}, // Image 0 reserved for block cursor.

// Graphic Cursor Image 1 (Solid White Arrow)
//**************************************************************//
 { 0X0f,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
   0X0f,0X0f,0X00,0X00,0X00,0X00,0X00,0X00,
   0X0f,0X0f,0X00,0X00,0X00,0X00,0X00,0X00,
   0X0f,0X0f,0X0f,0X00,0X00,0X00,0X00,0X00,
   0X0f,0X0f,0X0f,0X0f,0X00,0X00,0X00,0X00,
   0X0f,0X0f,0X0f,0X0f,0X00,0X00,0X00,0X00,
   0X0f,0X0f,0X0f,0X0f,0X0f,0X00,0X00,0X00,
   0X0f,0X0f,0X0f,0X0f,0X0f,0X00,0X00,0X00,
   0X0f,0X0f,0X0f,0X0f,0X0f,0X0f,0X00,0X00,
   0X0f,0X0f,0X0f,0X0f,0X0f,0X0f,0X0f,0X00,
   0X0f,0X0f,0X0f,0X0f,0X0f,0X0f,0X00,0X00,
   0X0f,0X0f,0X0f,0X0f,0X0f,0X00,0X00,0X00,
   0X0f,0X0f,0X00,0X0f,0X0f,0X00,0X00,0X00,
   0X0f,0X00,0X00,0X0f,0X0f,0X00,0X00,0X00,
   0X00,0X00,0X00,0X00,0X0f,0X0f,0X00,0X00,
   0X00,0X00,0X00,0X00,0X0f,0X0f,0X00,0X00
 },

// Graphic Cursor Image 2 (Hollow Arrow) 
//**************************************************************//
  {
   0X0f,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
   0X0f,0X0f,0X00,0X00,0X00,0X00,0X00,0X00,
   0X0f,0X0f,0X00,0X00,0X00,0X00,0X00,0X00,
   0X0f,0X00,0X0f,0X00,0X00,0X00,0X00,0X00,
   0X0f,0X00,0X00,0X0f,0X00,0X00,0X00,0X00,
   0X0f,0X00,0X00,0X0f,0X00,0X00,0X00,0X00,
   0X0f,0X00,0X00,0X00,0X0f,0X00,0X00,0X00,
   0X0f,0X00,0X00,0X00,0X0f,0X00,0X00,0X00,
   0X0f,0X00,0X00,0X00,0X00,0X0f,0X0f,0X00,
   0X0f,0X00,0X00,0X00,0X00,0X0f,0X0f,0X00,
   0X0f,0X00,0X00,0X00,0X0f,0X0f,0X00,0X00,
   0X0f,0X00,0X0f,0X0f,0X0f,0X00,0X00,0X00,
   0X0f,0X0f,0X0f,0X0f,0X0f,0X00,0X00,0X00,
   0X0f,0X00,0X00,0X0f,0X0f,0X00,0X00,0X00,
   0X00,0X00,0X00,0X00,0X0f,0X0f,0X00,0X00,
   0X00,0X00,0X00,0X00,0X0f,0X0f,0X00,0X00
  },

// Graphic Cursor Image 3 (I-beam) 
//**************************************************************//
  {
   0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
   0X0f,0X0f,0X0f,0X0f,0X0f,0X00,0X00,0X00,
   0X00,0X00,0X0f,0X00,0X00,0X00,0X00,0X00,
   0X00,0X00,0X0f,0X00,0X00,0X00,0X00,0X00,
   0X00,0X00,0X0f,0X00,0X00,0X00,0X00,0X00,
   0X00,0X00,0X0f,0X00,0X00,0X00,0X00,0X00,
   0X00,0X00,0X0f,0X00,0X00,0X00,0X00,0X00,
   0X00,0X00,0X0f,0X00,0X00,0X00,0X00,0X00,
   0X00,0X00,0X0f,0X00,0X00,0X00,0X00,0X00,
   0X00,0X00,0X0f,0X00,0X00,0X00,0X00,0X00,
   0X00,0X00,0X0f,0X00,0X00,0X00,0X00,0X00,
   0X00,0X00,0X0f,0X00,0X00,0X00,0X00,0X00,
   0X00,0X00,0X0f,0X00,0X00,0X00,0X00,0X00,
   0X00,0X00,0X0f,0X00,0X00,0X00,0X00,0X00,
   0X0f,0X0f,0X0f,0X0f,0X0f,0X00,0X00,0X00,
   0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00
  }
};

// Precomputed sinus and cosinus table from 0 to 359 degrees
// The tables are in Degrees not in Radian !
const float calcsi[360]={
 0.000001 ,                                                                                                                            //  0
 0.01745239 , 0.03489947 , 0.05233591 , 0.06975641 , 0.08715567 , 0.1045284 , 0.1218692 , 0.139173 , 0.1564343 , 0.173648 ,            // 1 à  10
 0.1908088 , 0.2079115 , 0.2249509 , 0.2419217 , 0.2588188 , 0.2756371 , 0.2923715 , 0.3090167 , 0.3255679 , 0.3420198 ,               // 11 à  20
 0.3583677 , 0.3746063 , 0.3907308 , 0.4067363 , 0.4226179 , 0.4383708 , 0.4539901 , 0.4694712 , 0.4848093 , 0.4999996 ,               // 21 à  30
 0.5150377 , 0.5299189 , 0.5446386 , 0.5591925 , 0.573576 , 0.5877848 , 0.6018146 , 0.615661 , 0.62932 , 0.6427872 ,                   // 31 à  40
 0.6560586 , 0.6691301 , 0.6819978 , 0.6946579 , 0.7071063 , 0.7193394 , 0.7313532 , 0.7431444 , 0.7547091 , 0.7660439 ,               // 41 à  50
 0.7771455 , 0.7880103 , 0.798635 , 0.8090165 , 0.8191515 , 0.8290371 , 0.8386701 , 0.8480476 , 0.8571668 , 0.8660249 ,                // 51 à  60
 0.8746193 , 0.8829472 , 0.8910061 , 0.8987936 , 0.9063074 , 0.913545 , 0.9205045 , 0.9271835 , 0.9335801 , 0.9396922 ,                // 61 à  70
 0.9455183 , 0.9510562 , 0.9563044 , 0.9612614 , 0.9659255 , 0.9702954 , 0.9743698 , 0.9781474 , 0.981627 , 0.9848075 ,                // 71 à  80
 0.9876881 , 0.9902679 , 0.992546 , 0.9945218 , 0.9961946 , 0.9975639 , 0.9986295 , 0.9993908 , 0.9998476 , 0.99999 ,                  // 81 à  90
 0.9998477 , 0.9993909 , 0.9986296 , 0.9975642 , 0.9961948 , 0.994522 , 0.9925463 , 0.9902682 , 0.9876886 , 0.984808 ,                 // 91 à  100
 0.9816275 , 0.9781479 , 0.9743704 , 0.9702961 , 0.9659262 , 0.9612621 , 0.9563052 , 0.9510571 , 0.9455191 , 0.9396932 ,               // 101 à  110
 0.933581 , 0.9271844 , 0.9205055 , 0.9135461 , 0.9063086 , 0.8987948 , 0.8910073 , 0.8829485 , 0.8746206 , 0.8660263 ,                // 111 à  120
 0.8571682 , 0.8480491 , 0.8386716 , 0.8290385 , 0.8191531 , 0.8090182 , 0.7986366 , 0.7880119 , 0.7771472 , 0.7660457 ,               // 121 à  130
 0.7547108 , 0.7431462 , 0.7313551 , 0.7193412 , 0.7071083 , 0.6946598 , 0.6819999 , 0.6691321 , 0.6560606 , 0.6427892 ,               // 131 à  140
 0.629322 , 0.6156631 , 0.6018168 , 0.5877869 , 0.5735782 , 0.5591948 , 0.5446408 , 0.5299212 , 0.5150401 , 0.5000019 ,                // 141 à  150
 0.4848116 , 0.4694737 , 0.4539925 , 0.4383733 , 0.4226205 , 0.4067387 , 0.3907333 , 0.3746087 , 0.3583702 , 0.3420225 ,               // 151 à  160
 0.3255703 , 0.3090193 , 0.2923741 , 0.2756396 , 0.2588214 , 0.2419244 , 0.2249534 , 0.2079142 , 0.1908116 , 0.1736506 ,               // 161 à  170
 0.156437 , 0.1391758 , 0.1218719 , 0.1045311 , 0.08715825 , 0.06975908 , 0.05233867 , 0.03490207 , 0.01745508 , 0.0277 ,              // 171 à  180
 -0.01744977 , -0.03489676 , -0.05233313 , -0.06975379 , -0.08715296 , -0.1045256 , -0.1218666 , -0.1391703 , -0.1564316 , -0.1736454 ,// 181 à  190
 -0.1908061 , -0.207909 , -0.2249483 , -0.241919 , -0.2588163 , -0.2756345 , -0.2923688 , -0.3090142 , -0.3255653 , -0.3420173 ,       // 191 à  200
 -0.3583652 , -0.3746038 , -0.3907282 , -0.4067339 , -0.4226155 , -0.4383683 , -0.4539878 , -0.4694688 , -0.4848068 , -0.4999973 ,     // 201 à  210
 -0.5150353 , -0.5299166 , -0.5446364 , -0.5591902 , -0.5735739 , -0.5877826 , -0.6018124 , -0.615659 , -0.6293178 , -0.642785 ,       // 211 à  220
 -0.6560566 , -0.6691281 , -0.6819958 , -0.694656 , -0.7071043 , -0.7193374 , -0.7313514 , -0.7431425 , -0.7547074 , -0.7660421 ,      // 221 à  230
 -0.7771439 , -0.7880087 , -0.7986334 , -0.8090149 , -0.8191499 , -0.8290355 , -0.8386687 , -0.8480463 , -0.8571655 , -0.8660236 ,     // 231 à  240
 -0.8746178 , -0.882946 , -0.8910049 , -0.8987925 , -0.9063062 , -0.9135439 , -0.9205033 , -0.9271825 , -0.9335791 , -0.9396913 ,      // 241 à  250
 -0.9455173 , -0.9510553 , -0.9563036 , -0.9612607 , -0.9659248 , -0.9702948 , -0.9743692 , -0.9781467 , -0.9816265 , -0.9848071 ,     // 251 à  260
 -0.9876878 , -0.9902675 , -0.9925456 , -0.9945215 , -0.9961944 , -0.9975638 , -0.9986293 , -0.9993907 , -0.9998476 , -0.99999 ,       // 261 à  270
 -0.9998478 , -0.9993909 , -0.9986298 , -0.9975643 , -0.9961951 , -0.9945223 , -0.9925466 , -0.9902686 , -0.987689 , -0.9848085 ,      // 271 à  280
 -0.981628 , -0.9781484 , -0.974371 , -0.9702968 , -0.965927 , -0.9612629 , -0.9563061 , -0.9510578 , -0.9455199 , -0.9396941 ,        // 281 à  290
 -0.933582 , -0.9271856 , -0.9205065 , -0.9135472 , -0.9063097 , -0.898796 , -0.8910086 , -0.8829498 , -0.8746218 , -0.8660276 ,       // 291 à  300
 -0.8571696 , -0.8480505 , -0.8386731 , -0.8290402 , -0.8191546 , -0.8090196 , -0.7986383 , -0.7880136 , -0.777149 , -0.7660476 ,      // 301 à  310
 -0.7547125 , -0.7431479 , -0.7313569 , -0.7193431 , -0.7071103 , -0.6946616 , -0.6820017 , -0.6691341 , -0.6560627 , -0.6427914 ,     // 311 à  320
 -0.6293243 , -0.6156651 , -0.6018188 , -0.5877892 , -0.5735805 , -0.5591971 , -0.5446434 , -0.5299233 , -0.5150422 , -0.5000043 ,     // 321 à  330
 -0.484814 , -0.4694761 , -0.4539948 , -0.4383755 , -0.4226228 , -0.4067413 , -0.3907359 , -0.3746115 , -0.3583725 , -0.3420248 ,      // 331 à  340
 -0.325573 , -0.3090219 , -0.2923768 , -0.2756425 , -0.2588239 , -0.2419269 , -0.2249561 , -0.2079169 , -0.1908143 , -0.1736531 ,      // 341 à  350
 -0.1564395 , -0.1391783 , -0.1218746 , -0.1045339 , -0.08716125 , -0.06976161 , -0.0523412 , -0.03490484 , -0.01745785 };             // 351 à  359

const float calcco[360]={
 0.99999 ,                                                                                                                            //  0
 0.9998477 , 0.9993908 , 0.9986295 , 0.9975641 , 0.9961947 , 0.9945219 , 0.9925462 , 0.9902681 , 0.9876884 , 0.9848078 ,              // 1 à  10
 0.9816272 , 0.9781477 , 0.9743701 , 0.9702958 , 0.9659259 , 0.9612617 , 0.9563049 , 0.9510566 , 0.9455186 , 0.9396928 ,              // 11 à  20
 0.9335806 , 0.927184 , 0.920505 , 0.9135456 , 0.906308 , 0.8987943 , 0.8910067 , 0.8829478 , 0.8746199 , 0.8660256 ,                 // 21 à  30
 0.8571675 , 0.8480483 , 0.8386709 , 0.8290379 , 0.8191524 , 0.8090173 , 0.7986359 , 0.7880111 , 0.7771463 , 0.7660448 ,              // 31 à  40
 0.75471 , 0.7431452 , 0.7313541 , 0.7193403 , 0.7071072 , 0.6946589 , 0.6819989 , 0.6691311 , 0.6560596 , 0.6427882 ,                // 41 à  50
 0.629321 , 0.6156621 , 0.6018156 , 0.5877859 , 0.5735771 , 0.5591936 , 0.5446398 , 0.52992 , 0.5150389 , 0.5000008 ,                 // 51 à  60
 0.4848104 , 0.4694724 , 0.4539914 , 0.438372 , 0.4226191 , 0.4067376 , 0.3907321 , 0.3746075 , 0.3583689 , 0.3420211 ,               // 61 à  70
 0.3255692 , 0.309018 , 0.2923728 , 0.2756384 , 0.2588201 , 0.241923 , 0.2249522 , 0.2079128 , 0.1908101 , 0.1736494 ,                // 71 à  80
 0.1564357 , 0.1391743 , 0.1218706 , 0.1045297 , 0.08715699 , 0.06975782 , 0.05233728 , 0.0349008 , 0.01745369 , 0.0138 ,             // 81 à  90
 -0.01745104 , -0.03489815 , -0.05233451 , -0.06975505 , -0.08715434 , -0.1045271 , -0.1218679 , -0.1391717 , -0.156433 , -0.1736467 ,// 91 à  100
 -0.1908075 , -0.2079102 , -0.2249495 , -0.2419204 , -0.2588175 , -0.2756359 , -0.2923701 , -0.3090155 , -0.3255666 , -0.3420185 ,    // 101 à  110
 -0.3583664 , -0.3746051 , -0.3907295 , -0.4067351 , -0.4226166 , -0.4383696 , -0.4539889 , -0.4694699 , -0.4848081 , -0.4999984 ,    // 111 à  120
 -0.5150366 , -0.5299177 , -0.5446375 , -0.5591914 , -0.5735749 , -0.5877837 , -0.6018136 , -0.6156599 , -0.6293188 , -0.6427862 ,    // 121 à  130
 -0.6560575 , -0.669129 , -0.6819969 , -0.6946569 , -0.7071053 , -0.7193384 , -0.7313522 , -0.7431435 , -0.7547083 , -0.7660431 ,     // 131 à  140
 -0.7771447 , -0.7880094 , -0.7986342 , -0.8090158 , -0.8191508 , -0.8290363 , -0.8386694 , -0.8480469 , -0.8571661 , -0.8660243 ,    // 141 à  150
 -0.8746186 , -0.8829465 , -0.8910055 , -0.898793 , -0.9063068 , -0.9135445 , -0.9205039 , -0.927183 , -0.9335796 , -0.9396918 ,      // 151 à  160
 -0.9455178 , -0.9510558 , -0.956304 , -0.9612611 , -0.9659252 , -0.9702951 , -0.9743695 , -0.978147 , -0.9816267 , -0.9848073 ,      // 161 à  170
 -0.9876879 , -0.9902677 , -0.9925459 , -0.9945216 , -0.9961945 , -0.9975639 , -0.9986294 , -0.9993907 , -0.9998476 , -0.99999 ,      // 171 à  180
 -0.9998477 , -0.9993909 , -0.9986297 , -0.9975642 , -0.9961949 , -0.9945222 , -0.9925465 , -0.9902685 , -0.9876888 , -0.9848083 ,    // 181 à  190
 -0.9816277 , -0.9781482 , -0.9743707 , -0.9702965 , -0.9659266 , -0.9612625 , -0.9563056 , -0.9510574 , -0.9455196 , -0.9396937 ,    // 191 à  200
 -0.9335815 , -0.927185 , -0.9205061 , -0.9135467 , -0.9063091 , -0.8987955 , -0.8910079 , -0.8829491 , -0.8746213 , -0.866027 ,      // 201 à  210
 -0.857169 , -0.8480497 , -0.8386723 , -0.8290394 , -0.8191538 , -0.8090189 , -0.7986375 , -0.7880127 , -0.7771481 , -0.7660466 ,     // 211 à  220
 -0.7547117 , -0.743147 , -0.731356 , -0.7193421 , -0.7071092 , -0.6946609 , -0.6820008 , -0.6691331 , -0.6560616 , -0.6427905 ,      // 221 à  230
 -0.6293229 , -0.6156641 , -0.6018178 , -0.5877882 , -0.5735794 , -0.5591961 , -0.5446419 , -0.5299222 , -0.5150412 , -0.5000032 ,    // 231 à  240
 -0.4848129 , -0.4694746 , -0.4539936 , -0.4383744 , -0.4226216 , -0.4067401 , -0.3907347 , -0.3746099 , -0.3583714 , -0.3420237 ,    // 241 à  250
 -0.3255718 , -0.3090207 , -0.2923756 , -0.2756409 , -0.2588227 , -0.2419256 , -0.2249549 , -0.2079156 , -0.1908126 , -0.1736519 ,    // 251 à  260
 -0.1564383 , -0.139177 , -0.1218734 , -0.1045326 , -0.08715951 , -0.06976035 , -0.05233994 , -0.03490358 , -0.01745659 , -0.0427 ,   // 261 à  270
 0.01744851 , 0.0348955 , 0.05233186 , 0.06975229 , 0.08715146 , 0.1045246 , 0.1218654 , 0.139169 , 0.1564303 , 0.1736439 ,           // 271 à  280
 0.1908047 , 0.2079078 , 0.224947 , 0.2419178 , 0.2588149 , 0.2756331 , 0.2923674 , 0.309013 , 0.3255641 , 0.3420161 ,                // 281 à  290
 0.3583638 , 0.3746024 , 0.3907273 , 0.4067327 , 0.4226143 , 0.4383671 , 0.4539864 , 0.4694674 , 0.4848059 , 0.4999962 ,              // 291 à  300
 0.5150342 , 0.5299154 , 0.5446351 , 0.559189 , 0.5735728 , 0.5877816 , 0.6018113 , 0.6156578 , 0.6293167 , 0.6427839 ,               // 301 à  310
 0.6560556 , 0.6691272 , 0.6819949 , 0.6946549 , 0.7071033 , 0.7193366 , 0.7313506 , 0.7431416 , 0.7547064 , 0.7660413 ,              // 311 à  320
 0.7771428 , 0.7880079 , 0.7986327 , 0.8090141 , 0.8191492 , 0.8290347 , 0.8386678 , 0.8480456 , 0.8571648 , 0.8660229 ,              // 321 à  330
 0.8746172 , 0.8829452 , 0.8910043 , 0.8987919 , 0.9063057 , 0.9135434 , 0.9205029 , 0.9271819 , 0.9335786 , 0.9396909 ,              // 331 à  340
 0.9455169 , 0.9510549 , 0.9563032 , 0.9612602 , 0.9659245 , 0.9702945 , 0.9743689 , 0.9781465 , 0.9816261 , 0.9848069 ,              // 341 à  350
 0.9876875 , 0.9902673 , 0.9925455 , 0.9945213 , 0.9961942 , 0.9975637 , 0.9986292 , 0.9993906 , 0.9998476 };                         // 351 à  359

// Text cursor struct.
typedef struct {
  bool     active = false;  // Cursor on/off.
  int16_t tCursor_x;       // Pixel based.
  int16_t tCursor_y;       // Pixel based.
  uint8_t  char_under_cursor[256]; // Char save array.
  bool     blink;           // Blink enable. True = blink.
  uint32_t blink_rate;	
  bool     toggle;          // Cursor blink togggle.
  uint8_t  x_start;         // Cursor x start (0-7).
  uint8_t  y_start;         // Cursor y start (0-15).
  uint8_t  x_end;           // Cursor y end (0-7).
  uint8_t  y_end;           // Cursor y end (0-15).
  uint8_t  color;           // Text cursor color.
} text_cursor;

// Graphic cursor struct.
typedef struct {
  bool     active = false;  // gCursor on/off.
  int16_t gCursor_x;       // Pixel based.
  int16_t gCursor_y;       // Pixel based.
  uint8_t  char_under_cursor[256]; // Char save array.
  uint8_t  x_start;         // Cursor x start (0-7).
  uint8_t  y_start;         // Cursor y start (0-15).
  uint8_t  x_end;           // Cursor y end (0-7).
  uint8_t  y_end;           // Cursor y end (0-15).
  uint8_t  type;            // Graphic type. 0=block, 1=arrow, 2=hollow arrow.
  uint8_t  color;           // Graphic cursor color.
} graphic_cursor;

// Cursor types
#define BLOCK_CURSOR 0
#define FILLED_ARROW 1
#define HOLLOW_ARROW 2
#define I_BEAM       3

typedef struct Gbuttons gbuttons_t;
/* Struct for graphic buttons */
/* Based on Adafruit graphic buttons */
/* Not completly implemented at this time */
struct Gbuttons {
  boolean initialzed;
  int16_t x;
  int16_t y;
  int16_t w;
  int16_t h;
  uint8_t outlinecolor;
  int8_t fillcolor;
  uint8_t textcolor;
  uint16_t textsize;
  boolean currstate;
  boolean laststate;
  char     label[10];
};

#define MaxPolyPoint    100

// 2D point structure
typedef struct {
	int16_t x;			// X Coordinate on screen
	int16_t y;			// Y Coordinate on screen
}Point2D;

// Polygon structure
typedef struct {
	Point2D		Center;				// Polygon Center (point where the polygon can rotate arround)
	Point2D		Pts[MaxPolyPoint];	// Points for the polygon
}PolyDef_t;

extern PolyDef_t PolySet;  // polygon data to declare in c file

//***************************************************************
// horizontal values must be divisible by 8 for correct operation
//***************************************************************
typedef struct {
  bool     active = false;
  uint32_t height;
  uint32_t vfp;
  uint32_t vsw;
  uint32_t vbp;
  uint32_t width;
  uint32_t hfp;
  uint32_t hsw;
  uint32_t hbp;
  uint32_t clk_num;
  uint32_t clk_den;
  // sync polarities: 0 = active high, 1 = active low
  uint32_t vsync_pol;
  uint32_t hsync_pol;
} vga_timing;

//********************************************************
// Timing modes. 1024x600x60 is not right yet...
//********************************************************
PROGMEM static const vga_timing t1280x720x60 = {
  .height=720, .vfp=13, .vsw=5, .vbp=12,
  .width=1280, .hfp=80, .hsw=40, .hbp=248,
  .clk_num=7425, .clk_den=2400, .vsync_pol=0, .hsync_pol=0
};

PROGMEM static const vga_timing t1024x768x60 = {
  .height=768, .vfp=3, .vsw=6, .vbp=29,
  .width=1024, .hfp=24, .hsw=136, .hbp=160,
  .clk_num=65, .clk_den=24, .vsync_pol=1, .hsync_pol=1
};

// Added
PROGMEM static const vga_timing t1024x600x60 = {
  .height=600, .vfp=1, .vsw=4, .vbp=23,
  .width=1024, .hfp=56, .hsw=160, .hbp=112,
  .clk_num=32, .clk_den=15, .vsync_pol=0, .hsync_pol=1
};

PROGMEM static const vga_timing t800x600x100 = {
  .height=600, .vfp=1, .vsw=3, .vbp=32,
  .width=800, .hfp=48, .hsw=88, .hbp=136,
  .clk_num=6818, .clk_den=2400, .vsync_pol=0, .hsync_pol=1
};

PROGMEM static const vga_timing t800x600x60 = {
  .height=600, .vfp=1, .vsw=4, .vbp=23,
  .width=800, .hfp=40, .hsw=128, .hbp=88,
  .clk_num=40, .clk_den=24, .vsync_pol=0, .hsync_pol=0
};

PROGMEM static const vga_timing t640x480x60 = {
  .height=480, .vfp=10, .vsw=2, .vbp=33,
  .width=640, .hfp=16, .hsw=96, .hbp=48,
  .clk_num=150, .clk_den=143, .vsync_pol=1, .hsync_pol=1
};

PROGMEM static const vga_timing t640x400x70 = {
  .height=400, .vfp=12, .vsw=2, .vbp=35,
  .width=640, .hfp=16, .hsw=96, .hbp=48,
  .clk_num=150, .clk_den=143, .vsync_pol=0, .hsync_pol=1
};

PROGMEM static const vga_timing t640x350x70 = {
  .height=350, .vfp=37, .vsw=2, .vbp=60,
  .width=640, .hfp=16, .hsw=96, .hbp=48,
  .clk_num=150, .clk_den=143, .vsync_pol=1, .hsync_pol=0
};

//***************************************************************
#define STRIDE_PADDING 16
#define SWAP(x,y) { (x)=(x)^(y); (y)=(x)^(y); (x)=(x)^(y); }
//***************************************************************
// FlexIO2VGA class
//***************************************************************
class FlexIO2VGA : public Print
{
public:

  FlexIO2VGA() {};

  void begin(const vga_timing& mode, bool half_height=false,
             bool half_width=false, unsigned int bpp=4);
  void stop(void);

  // wait parameter:
  // TRUE =  wait until previous frame ends and source is "current"
  // FALSE = queue it as the next framebuffer to draw, return immediately
  void set_next_buffer(const void* source, size_t pitch, bool wait);

  void wait_for_frame(void) {
    unsigned int count = frameCount;
    while (count == frameCount) yield();
  }

  void init();  // Currently unused
  void setScreenMode(const vga_timing& mode, bool half_height=false,
                                         bool half_width=false, unsigned int bpp=4);

  // Frame buffer and init methods
  void fbUpdate(bool wait);
  uint8_t *getFB(void) { return _fb; } // Return a pointer to the active
                                       // frame buffer
  size_t getPitch(void); // return stride size
  void setDoubleWidth(bool doubleWidth);  
  void setDoubleHeight(bool doubleHeight);  
  void getFbSize(int *width, int *height);
  
  uint16_t getGwidth(void);
  uint16_t getGheight(void);
  uint16_t getTwidth(void);
  uint16_t getTheight(void);
  uint8_t  getFGC(void) { return foreground_color; }
  uint8_t  getBGC(void) { return background_color; }

  // Initialize text settings
  void init_text_settings();

  // Software text cursor methods
  void initCursor(uint8_t xStart, uint8_t yStart, uint8_t xEnd,
                  uint8_t yEnd, bool blink, uint32_t blink_rate);	
  void tCursorOn(void);
  void tCursorOff(void);
  uint16_t tCursorX(void);
  uint16_t tCursorY(void);
  void drawTcursor(int color);
  void updateTCursor(int column, int line);
  void setCursorBlink(bool onOff);
  void setCursorType(uint8_t cursorType);
  void setTcursorColor(uint8_t cursorColor);  

  // Software graphic cursor methods
  void initGcursor(uint8_t type, uint8_t xStart, uint8_t yStart,
                   uint8_t xEnd, uint8_t yEnd);
  void gCursorOn(void);
  void gCursorOff(void);
  uint16_t gCursorX(void);
  uint16_t gCursorY(void);
  void drawGcursor(int color);
  void moveGcursor(int16_t column, int16_t line);
  void setGcursorColor(uint8_t cursorColor);  
  // Set block cursor dimensions: type = 0 for text, 1 for graphic.
  int8_t setBlkCursorDims(uint8_t xStart, uint8_t yStart, uint8_t xEnd,
                          uint8_t yEnd,uint8_t type);
  // low level frame buffer methods
  uint8_t getByte(uint32_t x, uint32_t y);
  void getChar(int16_t x, int16_t y, uint8_t *buf);
  void getGptr(int16_t x, int16_t y, uint8_t *buf);
  void putByte(uint32_t x, uint32_t y, uint8_t byte);
  void putChar(int16_t x, int16_t y, uint8_t *buf);
  void putGptr(int16_t x, int16_t y, uint8_t *buf);
  void writeVmem(uint8_t *buf, uint32_t vMem, uint32_t size);
  void readVmem(uint32_t vMem, uint8_t *buf, int32_t size);

  // Graphic methods
  void drawPixel(int16_t x, int16_t y, uint8_t fg);
  uint8_t getPixel(uint32_t x, uint32_t y);
  void drawHLine(int y, int x1, int x2, int color);
  void drawVLine(int y, int x1, int x2, int color);
  // Next two take 4 params and call drawLine() with 6 params.
  void draw_h_line(int16_t x1, int16_t y1, int16_t lenght, uint8_t color);
  void draw_v_line(int16_t x1, int16_t y1, int16_t lenght, uint8_t color);
  // delta = abs(v2-v1); sign = sign of (v2-v1);
  inline void delta_and_sign(int v1, int v2, int *delta, int *sign);
  void drawLine(int x0, int y0, int x1, int y1, int color, bool no_last_pixel);
  void drawRect(int x0, int y0, int x1, int y1, int color);
  void fillRect(int x0, int y0, int x1, int y1, int color);
  void fillQuad(int16_t centerx, int16_t centery, int16_t w, int16_t h, int16_t angle, uint8_t fillcolor);
  void drawQuad(int16_t centerx, int16_t centery, int16_t w, int16_t h, int16_t angle, uint8_t color);
  void drawCircle(float x, float y, float radius, float thickness, uint8_t color);
  void fillCircle(float xm, float ym, float r, uint8_t color);
  void drawTriangle(int x0, int y0, int x1, int y1, int x2, int y2, int color);
  void fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, int color);
  void drawArc(int xcenter,int ycenter,int xradius,int yradius,int startAngle,int endAngle);
  void drawEllipse(int16_t cx, int16_t cy, int16_t radius1, int16_t radius2, uint8_t color);
  void fillEllipse(int16_t cx, int16_t cy, int16_t radius1, int16_t radius2, uint8_t fillcolor);
  void drawRrect(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t r, uint8_t color);
  void fillRrect(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t r, uint8_t color);
  void drawpolygon(int16_t cx, int16_t cy, uint8_t bordercolor);
  void drawfullpolygon(int16_t cx, int16_t cy, uint8_t fillcolor, uint8_t bordercolor);
  void drawrotatepolygon(int16_t cx, int16_t cy, int16_t Angle, uint8_t fillcolor, uint8_t bordercolor, uint8_t filled);
  void drawBitmap(int16_t x_pos, int16_t y_pos, uint8_t *bitmap, int16_t bitmap_width, int16_t bitmap_height);
  void copy(int s_x, int s_y, int d_x, int d_y, int w, int h);

// Text methods
  void clear(uint8_t fg); // Clear full screen to fg color
  // define text print window. Width and height are in pixels
  // and cannot be smaller than font width and height
  void setPrintWindow(int x, int y, int width, int height);
  // With this one params are in character dimensions.
  void setPrintCWindow(uint8_t x, uint8_t y, uint8_t width, uint8_t height);
  void unsetPrintWindow();
  void clearPrintWindow();
  void scrollUpPrintWindow();
  void scrollDownPrintWindow();
  void scrollUp();
  void scrollDown();
  void scroll(int x, int y, int w, int h, int dx, int dy,int col);
  void drawText(int16_t x, int16_t y, const char * text, uint8_t fgcolor, uint8_t bgcolor);
  void drawText(int16_t x, int16_t y, const char * text, uint8_t fgcolor, uint8_t bgcolor, vga_text_direction dir);
  int  fontLoad(const char *filename, bool src);
  int  setFontSize(uint8_t fsize, bool runflag);  
  int  getFontWidth(void) { return font_width; }
  int  getFontHeight(void) { return font_height; }
  void setForegroundColor(int8_t fg_color); // RGBI format
  void setBackgroundColor(int8_t bg_color); // RGBI format
  void reverseVid(bool onOff);
  void textxy(int column, int line);
  int16_t getTextX(void) { return cursor_x; }
  int16_t getTextY(void) { return cursor_y; }
  uint8_t getTextBGC(void) { return background_color; }
  uint8_t getTextFGC(void) { return foreground_color; }
  void textColor(uint8_t fgc, uint8_t bgc);
  void setPromptSize(uint16_t ps); 
 
  virtual size_t write(const char *buffer, size_t size);
  virtual size_t write(uint8_t c);

  // Methods used by VT100 terminal and text editors
  void clreol(void);
  void clreos(void);
  void clrbol(void);
  void clrbos(void);
  void clrlin(void);

  void clearStatusLine(uint8_t bgc);
  void slWrite(int16_t x,  uint16_t fgcolor, uint16_t bgcolor, const char * text);

  uint16_t promp_size = 0;

  /* Button Functions */
  void initButton(struct Gbuttons *button, int16_t x, int16_t y, int16_t w, int16_t h,
		          uint8_t outline, uint8_t fill, uint8_t textcolor,
		          char *label, uint8_t textsize);
  void drawButton(struct Gbuttons *buttons, bool inverted);
  bool buttonContains(struct Gbuttons *buttons,int16_t x, int16_t y);
  void buttonPress(struct Gbuttons *buttons, bool p);
  bool buttonIsPressed(struct Gbuttons *buttons);
  bool buttonJustPressed(struct Gbuttons *buttons);
  bool buttonJustReleased(struct Gbuttons *buttons);
  
private:
  void set_clk(int num, int den);
  static void ISR(void);
  void TimerInterrupt(void);
  
 
  // Inline methods
  inline int clip_x(int x);
  inline int clip_y(int y);
  inline void drawHLineFast(int y, int x1, int x2, int color);
  inline void drawVLineFast(int x, int y1, int y2, int color);
  inline void drawLinex(int x0, int y0, int x1, int y1, int color) {
    drawLine(x0, y0, x1, y1, color, true);
  }
  inline void Vscroll(int x, int y, int w, int h, int dy ,int col);
  inline void Hscroll(int x, int y, int w, int h, int dx ,int col);

  // Private variables
  uint8_t foreground_color;
  uint8_t background_color;
  uint8_t SaveRVFGC; 
  uint8_t SaveRVBGC;
  bool transparent_background;

  short cursor_x;		// cursor x position in print window in CHARACTER
  short cursor_y;		// cursor y position in print window in CHARACTER
  short font_width;		// Font width: 8
  short font_height;	// Font height 8 or 16
  short print_window_x;	// x position in pixel of text window 
  short print_window_y;	// y position in pixel of text window
  short print_window_w;	// text window width in CHARACTER
  short print_window_h;	// text window height in CHARACTER

  uint8_t dma_chans[2];
  DMAChannel dma1,dma2,dmaswitcher;
  DMASetting dma_params;

  bool double_height;
  bool double_width;
  int32_t widthxbpp;
  int bpp;

  bool initialized = false;
  
  uint8_t *_fb = NULL;
  volatile unsigned int frameCount;
};

extern FlexIO2VGA vga4bit;
#endif
