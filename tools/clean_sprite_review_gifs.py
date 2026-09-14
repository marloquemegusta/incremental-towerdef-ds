from pathlib import Path

review = Path('artifacts/sprite-review').resolve()
for item in review.glob('*.gif'):
    item.unlink()
print('removed old GIFs')
