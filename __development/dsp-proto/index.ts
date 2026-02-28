type ParameterBuilder = {
	addUnary(
		address: number,
		identifier: string,
		label: string,
		defaultValue: number,
	): void;
	addEnum(
		address: number,
		identifier: string,
		label: string,
		defaultValue: number,
		valueStrings: string[],
	): void;
	addBool(
		address: number,
		identifier: string,
		label: string,
		defaultValue: boolean,
	): void;
};

type SynthesizerBase = {
	setupParameters(builder: ParameterBuilder): void;
	setParameter(address: number, value: number): void;
	prepare(sampleRate: number, maxFrameCount: number): void;
	noteOn(noteNumber: number, velocity: number): void;
	noteOff(noteNumber: number): void;
	process(buffer: Float32Array, frames: number): void;
};

export function createMySynthesizer(): SynthesizerBase {
	const state = {
		oscPitch: 0.5,
		oscVolume: 0.5,
		noteNumber: 0,
		gateOn: false,
		sampleRate: 0,
		phase: 0,
	};

	return {
		setupParameters(builder: ParameterBuilder): void {
			builder.addUnary(0, "oscPitch", "OSC Pitch", 0.5);
			builder.addUnary(1, "oscVolume", "OSC Volume", 0.5);
		},
		setParameter(address: number, value: number): void {
			switch (address) {
				case 0:
					state.oscPitch = value;
					break;
				case 1:
					state.oscVolume = value;
					break;
			}
		},
		prepare(sampleRate: number, _maxFrameCount: number): void {
			state.sampleRate = sampleRate;
		},
		noteOn(noteNumber: number, _velocity: number): void {
			state.noteNumber = noteNumber;
			state.gateOn = true;
		},
		noteOff(_noteNumber: number): void {
			state.gateOn = false;
		},
		process(buffer: Float32Array, frames: number): void {
			const noteNumber = state.noteNumber + (state.oscPitch * 2 - 1) * 12;
			const frequency = 440 * 2 ** ((noteNumber - 69) / 12);
			const phaseDelta = frequency / state.sampleRate;

			for (let i = 0; i < frames; i++) {
				state.phase += phaseDelta;
				if (state.phase >= 1) {
					state.phase -= 1;
				}
				const y = Math.sin(state.phase * 2 * Math.PI) * state.oscVolume;
				buffer[i] = y;
			}
		},
	};
}
