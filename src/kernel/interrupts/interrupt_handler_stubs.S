/*
#cant use C for these because a compiler will add in its own stack handling instructions, but the interrupt handlers use
#a diff stack frame format
*/

/*an interrupt vector's handler address is assigned based on an offset from vector 0's handler address, so we dedicate a fixed amount of bytes to each handler that is larger than the max handler instruction total byte size so we have a fixed known offset for each vector handler. the value 32 is chosen based on looking at the diassembly of the expansion of the interrupt_handler macro with the largest instruction byte size*/
.set VECTOR_X_INTERRUPT_VECTOR_HANDLER_MAX_BYTES, 32


interrupt_handler_skeleton:
pusha
#because we are using System V ABI for IA-32, all params will be passed on the stack
push %esp //the address where the userprocess_context_t struct data begins 
call interrupt_handler_dispatcher
mov %eax, %esp //reset the stack to where it was after we pushed all regs
popa
add $8, %esp #get rid of the vector number and real or fake error code on the stack
#call write_EOI
iret
 

.macro interrupt_handler vector has_error_code
.align VECTOR_X_INTERRUPT_VECTOR_HANDLER_MAX_BYTES
/*
we add a fake error code to the stack if there is not one so that we can always assume the stack frame for the
interrupt handler is the same size, because the return point to the interrupted program would otherwise sometimes 
pop an error code instead since not all interrupts/exceptions push error codes */
.if \has_error_code == 0 
        push $0xDEADBEEF
    .endif

#we will use another function to multiplex the proper C interrupt handler; we just pass the handler vector desired 
push $\vector
jmp interrupt_handler_skeleton
.endm

#vectors_with_errorcodes: .byte 0x8, 0xA, 0xB, 0xC, 0xD, 0xE, 0x11, 0x15

#the entry point for each interrupt handler
.global arch_defined_interrupts_start
.global vector_0
.align VECTOR_X_INTERRUPT_VECTOR_HANDLER_MAX_BYTES
arch_defined_interrupts_start:
vector_0:
    interrupt_handler 0 0
vector_1:
    interrupt_handler 1 0
vector_2:
    interrupt_handler 2 0
vector_3:
    interrupt_handler 3 0
vector_4:
    interrupt_handler 4 0
vector_5:
    interrupt_handler 5 0
vector_6:
    interrupt_handler 6 0
vector_7:
    interrupt_handler 7 0
vector_8:
    interrupt_handler 8 1
vector_9:
    interrupt_handler 9 0
vector_10:
    interrupt_handler 10 1
vector_11:
    interrupt_handler 11 1
vector_12:
    interrupt_handler 12 1
vector_13:
    interrupt_handler 13 1
vector_14:
    interrupt_handler 14 1
vector_15:
    interrupt_handler 15 0
vector_16:
    interrupt_handler 16 0
vector_17:
    interrupt_handler 17 1
vector_18:
    interrupt_handler 18 0
vector_19:
    interrupt_handler 19 0
vector_20:
    interrupt_handler 20 0
vector_21:
    interrupt_handler 21 1
vector_22:
    interrupt_handler 22 0
vector_23:
    interrupt_handler 23 0
vector_24:
    interrupt_handler 24 0
vector_25:
    interrupt_handler 25 0
vector_26:
    interrupt_handler 26 0
vector_27:
    interrupt_handler 27 0
vector_28:
    interrupt_handler 28 0
vector_29:
    interrupt_handler 29 0
vector_30:
    interrupt_handler 30 0
vector_31:
    interrupt_handler 31 0
vector_32:
    interrupt_handler 32 0
vector_33:
    interrupt_handler 33 0
vector_34:
    interrupt_handler 34 0
vector_35:
    interrupt_handler 35 0
vector_36:
    interrupt_handler 36 0
vector_37:
    interrupt_handler 37 0
vector_38:
    interrupt_handler 38 0
vector_39:
    interrupt_handler 39 0
vector_40:
    interrupt_handler 40 0
vector_41:
    interrupt_handler 41 0
vector_42:
    interrupt_handler 42 0
vector_43:
    interrupt_handler 43 0
vector_44:
    interrupt_handler 44 0
vector_45:
    interrupt_handler 45 0
vector_46:
    interrupt_handler 46 0
vector_47:
    interrupt_handler 47 0
vector_48:
    interrupt_handler 48 0
vector_49:
    interrupt_handler 49 0
vector_50:
    interrupt_handler 50 0
vector_51:
    interrupt_handler 51 0
vector_52:
    interrupt_handler 52 0
vector_53:
    interrupt_handler 53 0
vector_54:
    interrupt_handler 54 0
vector_55:
    interrupt_handler 55 0
vector_56:
    interrupt_handler 56 0
vector_57:
    interrupt_handler 57 0
vector_58:
    interrupt_handler 58 0
vector_59:
    interrupt_handler 59 0
vector_60:
    interrupt_handler 60 0
vector_61:
    interrupt_handler 61 0
vector_62:
    interrupt_handler 62 0
vector_63:
    interrupt_handler 63 0
vector_64:
    interrupt_handler 64 0
vector_65:
    interrupt_handler 65 0
vector_66:
    interrupt_handler 66 0
vector_67:
    interrupt_handler 67 0
vector_68:
    interrupt_handler 68 0
vector_69:
    interrupt_handler 69 0
vector_70:
    interrupt_handler 70 0
vector_71:
    interrupt_handler 71 0
vector_72:
    interrupt_handler 72 0
vector_73:
    interrupt_handler 73 0
vector_74:
    interrupt_handler 74 0
vector_75:
    interrupt_handler 75 0
vector_76:
    interrupt_handler 76 0
vector_77:
    interrupt_handler 77 0
vector_78:
    interrupt_handler 78 0
vector_79:
    interrupt_handler 79 0
vector_80:
    interrupt_handler 80 0
vector_81:
    interrupt_handler 81 0
vector_82:
    interrupt_handler 82 0
vector_83:
    interrupt_handler 83 0
vector_84:
    interrupt_handler 84 0
vector_85:
    interrupt_handler 85 0
vector_86:
    interrupt_handler 86 0
vector_87:
    interrupt_handler 87 0
vector_88:
    interrupt_handler 88 0
vector_89:
    interrupt_handler 89 0
vector_90:
    interrupt_handler 90 0
vector_91:
    interrupt_handler 91 0
vector_92:
    interrupt_handler 92 0
vector_93:
    interrupt_handler 93 0
vector_94:
    interrupt_handler 94 0
vector_95:
    interrupt_handler 95 0
vector_96:
    interrupt_handler 96 0
vector_97:
    interrupt_handler 97 0
vector_98:
    interrupt_handler 98 0
vector_99:
    interrupt_handler 99 0
vector_100:
    interrupt_handler 100 0
vector_101:
    interrupt_handler 101 0
vector_102:
    interrupt_handler 102 0
vector_103:
    interrupt_handler 103 0
vector_104:
    interrupt_handler 104 0
vector_105:
    interrupt_handler 105 0
vector_106:
    interrupt_handler 106 0
vector_107:
    interrupt_handler 107 0
vector_108:
    interrupt_handler 108 0
vector_109:
    interrupt_handler 109 0
vector_110:
    interrupt_handler 110 0
vector_111:
    interrupt_handler 111 0
vector_112:
    interrupt_handler 112 0
vector_113:
    interrupt_handler 113 0
vector_114:
    interrupt_handler 114 0
vector_115:
    interrupt_handler 115 0
vector_116:
    interrupt_handler 116 0
vector_117:
    interrupt_handler 117 0
vector_118:
    interrupt_handler 118 0
vector_119:
    interrupt_handler 119 0
vector_120:
    interrupt_handler 120 0
vector_121:
    interrupt_handler 121 0
vector_122:
    interrupt_handler 122 0
vector_123:
    interrupt_handler 123 0
vector_124:
    interrupt_handler 124 0
vector_125:
    interrupt_handler 125 0
vector_126:
    interrupt_handler 126 0
vector_127:
    interrupt_handler 127 0
vector_128:
    interrupt_handler 128 0
vector_129:
    interrupt_handler 129 0
vector_130:
    interrupt_handler 130 0
vector_131:
    interrupt_handler 131 0
vector_132:
    interrupt_handler 132 0
vector_133:
    interrupt_handler 133 0
vector_134:
    interrupt_handler 134 0
vector_135:
    interrupt_handler 135 0
vector_136:
    interrupt_handler 136 0
vector_137:
    interrupt_handler 137 0
vector_138:
    interrupt_handler 138 0
vector_139:
    interrupt_handler 139 0
vector_140:
    interrupt_handler 140 0
vector_141:
    interrupt_handler 141 0
vector_142:
    interrupt_handler 142 0
vector_143:
    interrupt_handler 143 0
vector_144:
    interrupt_handler 144 0
vector_145:
    interrupt_handler 145 0
vector_146:
    interrupt_handler 146 0
vector_147:
    interrupt_handler 147 0
vector_148:
    interrupt_handler 148 0
vector_149:
    interrupt_handler 149 0
vector_150:
    interrupt_handler 150 0
vector_151:
    interrupt_handler 151 0
vector_152:
    interrupt_handler 152 0
vector_153:
    interrupt_handler 153 0
vector_154:
    interrupt_handler 154 0
vector_155:
    interrupt_handler 155 0
vector_156:
    interrupt_handler 156 0
vector_157:
    interrupt_handler 157 0
vector_158:
    interrupt_handler 158 0
vector_159:
    interrupt_handler 159 0
vector_160:
    interrupt_handler 160 0
vector_161:
    interrupt_handler 161 0
vector_162:
    interrupt_handler 162 0
vector_163:
    interrupt_handler 163 0
vector_164:
    interrupt_handler 164 0
vector_165:
    interrupt_handler 165 0
vector_166:
    interrupt_handler 166 0
vector_167:
    interrupt_handler 167 0
vector_168:
    interrupt_handler 168 0
vector_169:
    interrupt_handler 169 0
vector_170:
    interrupt_handler 170 0
vector_171:
    interrupt_handler 171 0
vector_172:
    interrupt_handler 172 0
vector_173:
    interrupt_handler 173 0
vector_174:
    interrupt_handler 174 0
vector_175:
    interrupt_handler 175 0
vector_176:
    interrupt_handler 176 0
vector_177:
    interrupt_handler 177 0
vector_178:
    interrupt_handler 178 0
vector_179:
    interrupt_handler 179 0
vector_180:
    interrupt_handler 180 0
vector_181:
    interrupt_handler 181 0
vector_182:
    interrupt_handler 182 0
vector_183:
    interrupt_handler 183 0
vector_184:
    interrupt_handler 184 0
vector_185:
    interrupt_handler 185 0
vector_186:
    interrupt_handler 186 0
vector_187:
    interrupt_handler 187 0
vector_188:
    interrupt_handler 188 0
vector_189:
    interrupt_handler 189 0
vector_190:
    interrupt_handler 190 0
vector_191:
    interrupt_handler 191 0
vector_192:
    interrupt_handler 192 0
vector_193:
    interrupt_handler 193 0
vector_194:
    interrupt_handler 194 0
vector_195:
    interrupt_handler 195 0
vector_196:
    interrupt_handler 196 0
vector_197:
    interrupt_handler 197 0
vector_198:
    interrupt_handler 198 0
vector_199:
    interrupt_handler 199 0
vector_200:
    interrupt_handler 200 0
vector_201:
    interrupt_handler 201 0
vector_202:
    interrupt_handler 202 0
vector_203:
    interrupt_handler 203 0
vector_204:
    interrupt_handler 204 0
vector_205:
    interrupt_handler 205 0
vector_206:
    interrupt_handler 206 0
vector_207:
    interrupt_handler 207 0
vector_208:
    interrupt_handler 208 0
vector_209:
    interrupt_handler 209 0
vector_210:
    interrupt_handler 210 0
vector_211:
    interrupt_handler 211 0
vector_212:
    interrupt_handler 212 0
vector_213:
    interrupt_handler 213 0
vector_214:
    interrupt_handler 214 0
vector_215:
    interrupt_handler 215 0
vector_216:
    interrupt_handler 216 0
vector_217:
    interrupt_handler 217 0
vector_218:
    interrupt_handler 218 0
vector_219:
    interrupt_handler 219 0
vector_220:
    interrupt_handler 220 0
vector_221:
    interrupt_handler 221 0
vector_222:
    interrupt_handler 222 0
vector_223:
    interrupt_handler 223 0
vector_224:
    interrupt_handler 224 0
vector_225:
    interrupt_handler 225 0
vector_226:
    interrupt_handler 226 0
vector_227:
    interrupt_handler 227 0
vector_228:
    interrupt_handler 228 0
vector_229:
    interrupt_handler 229 0
vector_230:
    interrupt_handler 230 0
vector_231:
    interrupt_handler 231 0
vector_232:
    interrupt_handler 232 0
vector_233:
    interrupt_handler 233 0
vector_234:
    interrupt_handler 234 0
vector_235:
    interrupt_handler 235 0
vector_236:
    interrupt_handler 236 0
vector_237:
    interrupt_handler 237 0
vector_238:
    interrupt_handler 238 0
vector_239:
    interrupt_handler 239 0
vector_240:
    interrupt_handler 240 0
vector_241:
    interrupt_handler 241 0
vector_242:
    interrupt_handler 242 0
vector_243:
    interrupt_handler 243 0
vector_244:
    interrupt_handler 244 0
vector_245:
    interrupt_handler 245 0
vector_246:
    interrupt_handler 246 0
vector_247:
    interrupt_handler 247 0
vector_248:
    interrupt_handler 248 0
vector_249:
    interrupt_handler 249 0
vector_250:
    interrupt_handler 250 0
vector_251:
    interrupt_handler 251 0
vector_252:
    interrupt_handler 252 0
vector_253:
    interrupt_handler 253 0
vector_254:
    interrupt_handler 254 0
vector_255:
    interrupt_handler 255 0



