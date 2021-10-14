/*
	SPDX-FileCopyrightText: 2007-2009 Sergio Pistone <sergio_pistone@yahoo.com.ar>

	SPDX-License-Identifier: GPL-2.0-or-later

	@category Examples
	@name Iterate Selected Lines
	@version 1.0
	@summary Example script to iterate over selected lines and show their text.
	@author SubtitleComposer Team
*/

function RangesIterator(rangeList, forward) {
	this.forward = forward === undefined ? true : forward;
	this.rangeList = rangeList;
	this.rangeIndex = this.forward ? -1 : this.rangeList.rangesCount()

	this.current = function() {
		return this.rangeIndex >= 0 && this.rangeIndex < this.rangeList.rangesCount() ?
 			this.rangeList.range(this.rangeIndex) :
			null;
	};

	this.hasNext = function() {
		return this.forward ?
			this.rangeIndex < this.rangeList.rangesCount() - 1 :
			this.rangeIndex > 0;
	};

	this.next = function() {
		this.rangeIndex += this.forward ? 1 : -1;
		return this.current();
	};
}

function LinesIterator(rangeList, forward) {
	this.forward = forward === undefined ? true : forward;
	this.rangesIt = new RangesIterator(rangeList, this.forward);
	this.lineIndex = -1;
	this.subtitle = subtitle.instance();

	this.current = function() {
		let range = this.rangesIt.current();
		return range != null && this.lineIndex >= range.start() && this.lineIndex < range.end()
				? this.subtitle.line(this.lineIndex) : null;
	};

	this.hasNext = function() {
		return this.rangesIt.hasNext() || (this.rangesIt.current() &&
			(this.forward ? this.lineIndex < this.rangesIt.current().end() : this.lineIndex > this.rangesIt.current().start())
			);
	};

	this.next = function() {
		let currentRange = this.rangesIt.current();
		if(currentRange == null || (this.lineIndex == (forward ? currentRange.end() : currentRange.start()))) {
			if(!this.rangesIt.hasNext())
				return null;
			currentRange = this.rangesIt.next();
			this.lineIndex = this.forward ? currentRange.start() : currentRange.end();
		} else {
			this.lineIndex += this.forward ? 1 : -1;
		}
		return this.subtitle.line(this.lineIndex);
	};
}

let rangesIt = new RangesIterator(ranges.newSelectionRangeList(), true);
while(rangesIt.hasNext()) {
	let range = rangesIt.next();
	debug.information( range.start().toString() + ":" + range.end().toString() );
}

let linesIt = new LinesIterator( ranges.newSelectionRangeList(), false );
while(linesIt.hasNext()) {
	let line = linesIt.next();
	debug.information("Plain text: " + line.plainPrimaryText() + "\n\nRich Text: " + line.richPrimaryText());
}
