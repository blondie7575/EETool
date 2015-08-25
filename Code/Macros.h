//
//  Macros.h
//  EETool
//
//  Created by Quinn Dunki on 8/22/15.
//  Copyright (c) 2015 Quinn Dunki. All rights reserved.
//

#ifndef EETool_Macros_h
#define EETool_Macros_h

#define QDDRB(n) (1<<(DDB##n))
#define QBPINBIT(n) PB##n
#define READPINB(pin) ((PINB & (1<<(QBPINBIT(pin)))) > 0)
#define SETB_HI(pin) PORTB |= (1<<(QBPINBIT(pin)))
#define SETB_LO(pin) PORTB &= ~(1<<(QBPINBIT(pin)))
#define PULSEB(pin) SETB_LO(pin); _delay_ms(100); SETB_HI(pin);

#define QDDRC(n) (1<<(DDC##n))
#define QCPINBIT(n) PC##n
#define READPINC(pin) ((PINC & (1<<(QCPINBIT(pin)))) > 0)
#define SETC_HI(pin) PORTC |= (1<<(QCPINBIT(pin)))
#define SETC_LO(pin) PORTC &= ~(1<<(QCPINBIT(pin)))
#define PULSEC(pin) SETC_LO(pin); _delay_ms(50); SETC_HI(pin);

#define QDDRD(n) (1<<(DDD##n))
#define QDPINBIT(n) PD##n
#define READPIND(pin) ((PIND & (1<<(QDPINBIT(pin)))) > 0)
#define SETD_HI(pin) PORTD |= (1<<(QDPINBIT(pin)))
#define SETD_LO(pin) PORTD &= ~(1<<(QDPINBIT(pin)))
#define PULSED(pin) SETD_LO(pin); _delay_ms(100); SETD_HI(pin);

#define QDDRE(n) (1<<(DDE##n))
#define QEPINBIT(n) PE##n
#define READPINE(pin) ((PINE & (1<<(QEPINBIT(pin)))) > 0)
#define SETE_HI(pin) PORTE |= (1<<(QEPINBIT(pin)))
#define SETE_LO(pin) PORTE &= ~(1<<(QEPINBIT(pin)))
#define PULSEE(pin) SETE_LO(pin); _delay_ms(100); SETE_HI(pin);

#define QDDRF(n) (1<<(DDF##n))
#define QFPINBIT(n) PF##n
#define READPINF(pin) ((PINF & (1<<(QFPINBIT(pin)))) > 0)
#define SETF_HI(pin) PORTF |= (1<<(QFPINBIT(pin)))
#define SETF_LO(pin) PORTF &= ~(1<<(QFPINBIT(pin)))
#define PULSEF(pin) SETF_LO(pin); _delay_ms(100); SETF_HI(pin);


#endif
