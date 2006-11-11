def subbanese(text)
  lowercase = %w[e q > p a j 6 y ! r >| I w u o d b j s + n  ^ m >< h z]
  uppercase = %w[V 8 ) (| 3 j & H I £ }| 7 W N 0 d Ô j S L n A M X h Z]
  mapping = {}
  ('a'..'z').each_with_index { |o, i| mapping[o] = lowercase[i] }
  ('A'..'Z').each_with_index { |o, i| mapping[o] = uppercase[i] }
  mapping.merge!({'1' => '|', '2' => '5', '3' => 'e', '4' => 'h', '5' => '2',
                  '6' => '9', '7' => 'L', '8' => '8', '9' => '6', '0' => '0',
                  '!' => '¡', '?' => '¿'})
  text.split(//).inject("") { |memo, o| (mapping[o] || o) + memo }
end
